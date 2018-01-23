/*
 * _ZEDobstacle.cpp
 *
 *  Created on: Jan 6, 2017
 *      Author: yankai
 */

#include "_ZEDdistance.h"

namespace kai
{

_ZEDdistance::_ZEDdistance()
{
#ifdef USE_ZED
	m_pZed = NULL;
	m_bZEDready = false;
#endif
	m_pMatrix = NULL;
	m_nFilter = 0;
	m_dBlend = 0.5;
	m_mDim.x = 10;
	m_mDim.y = 10;
}

_ZEDdistance::~_ZEDdistance()
{
	reset();
}

bool _ZEDdistance::init(void* pKiss)
{
	IF_F(!_ThreadBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;
	pK->m_pInst = this;

	string presetDir = "";
	F_INFO(pK->root()->o("APP")->v("presetDir", &presetDir));
	F_INFO(pK->v("dBlend", &m_dBlend));
	F_INFO(pK->v("matrixW", &m_mDim.x));
	F_INFO(pK->v("matrixH", &m_mDim.y));

	m_nFilter = m_mDim.area();
	IF_F(m_nFilter >= N_FILTER);

	Kiss* pC;
	int i;

	//filter
	pC = pK->o("medianFilter");
	IF_F(pC->empty());

	for (i = 0; i < m_nFilter; i++)
	{
		m_pFilteredMatrix[i] = new Median();
		IF_F(!m_pFilteredMatrix[i]->init(pC));
	}
	m_pMatrix = new Frame();

	return true;
}

void _ZEDdistance::reset(void)
{
	DEL(m_pMatrix);

	for (int i = 0; i < m_nFilter; i++)
		DEL(m_pFilteredMatrix[i]);
}

bool _ZEDdistance::link(void)
{
	IF_F(!this->_ThreadBase::link());
	Kiss* pK = (Kiss*) m_pKiss;

	string iName = "";

#ifdef USE_ZED
	F_INFO(pK->v("_ZED", &iName));
	m_pZed = (_ZED*) (pK->root()->getChildInstByName(&iName));
	if(!m_pZed)
	{
		LOG_E(iName << " not found");
		return false;
	}

	vDouble2 range = m_pZed->range();
	m_rMin = range.x;
	m_rMax = range.y;
#endif

	return true;
}

bool _ZEDdistance::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _ZEDdistance::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		detect();

		this->autoFPSto();
	}
}

void _ZEDdistance::detect(void)
{
#ifdef USE_ZED
	NULL_(m_pZed);
	IF_(!m_pZed->isOpened());

	Frame* pDepth = m_pZed->depth();
	NULL_(pDepth);
	if(pDepth->empty())
	{
		m_bZEDready = false;
		return;
	}
	else
	{
		m_bZEDready = true;
	}

	m_pMatrix->getResizedOf(m_pZed->depth(), m_mDim.x, m_mDim.y);
	Mat* pM = m_pMatrix->getCMat();

	int i,j;
	for(i=0;i<m_mDim.y;i++)
	{
		for(j=0;j<m_mDim.x;j++)
		{
			m_pFilteredMatrix[i*m_mDim.x+j]->input((double)pM->at<float>(i,j));
		}
	}
#endif
}

double _ZEDdistance::d(vInt4* pROI, vInt2* pPos)
{
#ifdef USE_ZED
	if(!m_pZed)return -1.0;
#endif

	if(!pROI)return -1.0;

	double dMin = m_rMax;
	int i,j;
	for(i=pROI->y;i<pROI->w;i++)
	{
		for(j=pROI->x;j<pROI->z;j++)
		{
			double dCell = m_pFilteredMatrix[i*m_mDim.x+j]->v();
			IF_CONT(dCell < m_rMin);
			IF_CONT(dCell > m_rMax);
			IF_CONT(dCell > dMin);

			dMin = dCell;
			if(pPos)
			{
				pPos->x = j;
				pPos->y = i;
			}
		}
	}

	return dMin;
}

double _ZEDdistance::d(vDouble4* pROI, vInt2* pPos)
{
#ifdef USE_ZED
	if(!m_pZed)return -1.0;
#endif
	if(!pROI)return -1.0;

	vInt4 iR;
	iR.x = pROI->x * m_mDim.x;
	iR.y = pROI->y * m_mDim.y;
	iR.z = pROI->z * m_mDim.x;
	iR.w = pROI->w * m_mDim.y;

	if (iR.x < 0)
		iR.x = 0;
	if (iR.y < 0)
		iR.y = 0;
	if (iR.z >= m_mDim.x)
		iR.z = m_mDim.x - 1;
	if (iR.w >= m_mDim.y)
		iR.w = m_mDim.y - 1;

	double dMin = m_rMax;
	int i,j;
	for(i=iR.y;i<iR.w;i++)
	{
		for(j=iR.x;j<iR.z;j++)
		{
			double dCell = m_pFilteredMatrix[i*m_mDim.x+j]->v();
			IF_CONT(dCell < m_rMin);
			IF_CONT(dCell > m_rMax);
			IF_CONT(dCell > dMin);

			dMin = dCell;
			if(pPos)
			{
				pPos->x = j;
				pPos->y = i;
			}
		}
	}

	return dMin;
}

vInt2 _ZEDdistance::matrixDim(void)
{
	return m_mDim;
}

DIST_SENSOR_TYPE _ZEDdistance::type(void)
{
	return dsZED;
}

bool _ZEDdistance::bReady(void)
{
#ifdef USE_ZED
	return m_bZEDready;
#else
	return false;
#endif
}

bool _ZEDdistance::draw(void)
{
	IF_F(!this->_ThreadBase::draw());
	Mat* pMat = ((Window*) this->m_pWindow)->getFrame()->getCMat();
	IF_F(pMat->empty());

#ifdef USE_ZED
	NULL_F(m_pZed);
#endif

	IF_F(m_pMatrix->empty());

	Mat mM = *m_pMatrix->getCMat();
	IF_F(mM.empty());

	double normD;
	double baseD = 255.0/(m_rMax - m_rMin);

    Mat filterM = Mat::zeros(Size(m_mDim.x,m_mDim.y), CV_8UC1);
	int i,j;
	for(i=0;i<m_mDim.y;i++)
	{
		for(j=0;j<m_mDim.x;j++)
		{
			normD = m_pFilteredMatrix[i*m_mDim.x+j]->v();
			normD = (normD - m_rMin) * baseD;
			filterM.at<uchar>(i,j) = 255 - (uchar)normD;
		}
	}

	Mat mA;
    mA = Mat::zeros(Size(m_mDim.x,m_mDim.y), CV_8UC1);
	vector<Mat> channels;
    channels.push_back(mA);
    channels.push_back(mA);
    channels.push_back(filterM);
    Mat mB;
    cv::merge(channels, mB);
	cv::resize(mB, mA, Size(pMat->cols, pMat->rows),0,0,INTER_NEAREST);
	cv::addWeighted(*pMat, 1.0, mA, m_dBlend, 0.0, *pMat);

	return true;
}

}
