/*
 *  Created on: Sept 28, 2016
 *      Author: yankai
 */
#include "_DetectNet.h"

#ifdef USE_TENSORRT

namespace kai
{

_DetectNet::_DetectNet()
{
	m_pRGBA = NULL;
	m_pRGBAf = NULL;
	m_coverageThr = 0.5;
	m_minSize = 0.0;
	m_maxSize = 1.0;
	m_area.init();
	m_area.z = 1.0;
	m_area.w = 1.0;

	m_pDN = NULL;
	m_nBox = 0;
	m_nBoxMax = 0;
	m_nClass = 0;

	m_bbCPU = NULL;
	m_bbCUDA = NULL;
	m_confCPU = NULL;
	m_confCUDA = NULL;

	m_className = "";
}

_DetectNet::~_DetectNet()
{
	DEL(m_pRGBA);
	DEL(m_pRGBAf);
}

bool _DetectNet::init(void* pKiss)
{
	IF_F(!this->_DetectorBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;
	pK->m_pInst = this;

	KISSm(pK, coverageThr);
	KISSm(pK, className);
	KISSm(pK, minSize);
	KISSm(pK, maxSize);

	F_INFO(pK->v("left", &m_area.x));
	F_INFO(pK->v("top", &m_area.y));
	F_INFO(pK->v("right", &m_area.z));
	F_INFO(pK->v("bottom", &m_area.w));

	m_pRGBA = new Frame();
	m_pRGBAf = new Frame();

	bSetActive(true);

	return true;
}

bool _DetectNet::link(void)
{
	IF_F(!this->_DetectorBase::link());
	Kiss* pK = (Kiss*) m_pKiss;

	return true;
}

bool _DetectNet::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		LOG_E(retCode);
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _DetectNet::update(void)
{
	m_pDN = detectNet::Create(
			m_modelFile.c_str(),
			m_trainedFile.c_str(),
			m_meanFile.c_str(),
			m_minConfidence);
	NULL_(m_pDN);

	m_nBoxMax = m_pDN->GetMaxBoundingBoxes();
	m_nClass = m_pDN->GetNumClasses();
	m_pDN->SetThreshold(m_coverageThr);

	IF_(	!cudaAllocMapped((void** )&m_bbCPU, (void** )&m_bbCUDA,
					m_nBoxMax * sizeof(float4)));
	IF_(	!cudaAllocMapped((void** )&m_confCPU, (void** )&m_confCUDA,
					m_nBoxMax * m_nClass * sizeof(float)));

	while (m_bThreadON)
	{
		this->autoFPSfrom();

		m_obj.update();
		detect();
		mergeDetector();

		this->autoFPSto();
	}

}

void _DetectNet::detect(void)
{
	NULL_(m_pVision);
	NULL_(m_pDN);

	Frame* pBGR = m_pVision->bgr();
	NULL_(pBGR);
	IF_(pBGR->empty());
	IF_(m_pRGBA->isNewerThan(pBGR));

	m_pRGBA->getRGBAOf(pBGR);
	GpuMat* pGMat = m_pRGBA->getGMat();
	IF_(pGMat->empty());

	GpuMat fGMat;
	pGMat->convertTo(fGMat, CV_32FC4);

	m_nBox = m_nBoxMax;

	IF_(!m_pDN->Detect((float* )fGMat.data, fGMat.cols, fGMat.rows, m_bbCPU, &m_nBox, m_confCPU));

	int camArea = fGMat.cols * fGMat.rows;
	int minSize = camArea * m_minSize;
	int maxSize = camArea * m_maxSize;

	m_tStamp = getTimeUsec();

	OBJECT obj;
	for (int n = 0; n < m_nBox; n++)
	{
		obj.init();
		double prob = (double)m_confCPU[n*2];
		IF_CONT(prob < m_minConfidence);

		obj.addClass((int)m_confCPU[n*2+1]);
		obj.m_tStamp = m_tStamp;

		float* bb = m_bbCPU + (n * 4);
		obj.m_bbox.x = (int) bb[0];
		obj.m_bbox.y = (int) bb[1];
		obj.m_bbox.z = (int) bb[2];
		obj.m_bbox.w = (int) bb[3];
		obj.m_camSize.x = fGMat.cols;
		obj.m_camSize.y = fGMat.rows;
		if(obj.m_bbox.x < 0)obj.m_bbox.x = 0;
		if(obj.m_bbox.y < 0)obj.m_bbox.y = 0;
		if(obj.m_bbox.z > obj.m_camSize.x)obj.m_bbox.z = obj.m_camSize.x;
		if(obj.m_bbox.w > obj.m_camSize.y)obj.m_bbox.w = obj.m_camSize.y;
		obj.i2fBBox();

		int oSize = obj.m_bbox.area();
		IF_CONT(oSize < minSize);
		IF_CONT(oSize > maxSize);

		add(&obj);

		LOG_I("BBox: "<< obj.m_iClass << " " << prob);
	}

}

bool _DetectNet::draw(void)
{
	IF_F(!this->_DetectorBase::draw());

	return true;
}

}

#endif

