/*
 * CamBase.cpp
 *
 *  Created on: Aug 22, 2015
 *      Author: yankai
 */

#include "_VisionBase.h"

namespace kai
{

_VisionBase::_VisionBase()
{
	m_bOpen = false;
	m_type = unknownStream;
	m_orientation = 0;
	m_width = 1280;
	m_height = 720;
	m_centerH = 640;
	m_centerV = 360;
	m_bGimbal = false;
	m_isoScale = 1.0;
	m_rotTime = 0;
	m_rotPrev = 0;
	m_angleH = 60;
	m_angleV = 60;
	m_bFlip = false;

	m_pBGR = NULL;
	m_pHSV = NULL;
	m_pGray = NULL;
	m_pDepth = NULL;
	m_pDepthNorm = NULL;
	m_depthNormInt.x = 0.0;
	m_depthNormInt.y = 15.0;
}

_VisionBase::~_VisionBase()
{
	reset();
}

bool _VisionBase::init(void* pKiss)
{
	IF_F(!this->_ThreadBase::init(pKiss));
	Kiss* pK = (Kiss*)pKiss;
	pK->m_pInst = this;

	KISSm(pK,width);
	KISSm(pK,height);
	KISSm(pK,angleV);
	KISSm(pK,angleH);
	KISSm(pK,bGimbal);
	KISSm(pK,isoScale);
	KISSm(pK,bFlip);
	KISSim(pK,orientation);
	F_INFO(pK->v("depthNormFrom", &m_depthNormInt.x));
	F_INFO(pK->v("depthNormTo", &m_depthNormInt.y));

	m_pBGR = new Frame();

	bool bParam = false;
	F_INFO(pK->v("bGray", &bParam));
	if (bParam)
		m_pGray = new Frame();

	bParam = false;
	F_INFO(pK->v("bHSV", &bParam));
	if (bParam)
		m_pHSV = new Frame();

	bParam = false;
	F_INFO(pK->v("bDepthNorm", &bParam));
	if (bParam)
	{
		m_pDepthNorm = new Frame();
	}

	m_bOpen = false;
	return true;
}

void _VisionBase::reset(void)
{
	this->_ThreadBase::reset();

	DEL(m_pBGR);
	DEL(m_pHSV);
	DEL(m_pGray);
	DEL(m_pDepth);
	DEL(m_pDepthNorm);
}

Frame* _VisionBase::bgr(void)
{
	return m_pBGR;
}

Frame* _VisionBase::hsv(void)
{
	return m_pHSV;
}

Frame* _VisionBase::gray(void)
{
	return m_pGray;
}

Frame* _VisionBase::depth(void)
{
	return m_pDepth;
}

Frame* _VisionBase::depthNorm(void)
{
	return m_pDepthNorm;
}

uint8_t _VisionBase::getOrientation(void)
{
	return m_orientation;
}

void _VisionBase::info(vInt2* pSize, vInt2* pCenter, vInt2* pAngle)
{
	if(pSize)
	{
		pSize->x = m_width;
		pSize->y = m_height;
	}

	if(pCenter)
	{
		pCenter->x = m_centerH;
		pCenter->y = m_centerV;
	}

	if(pAngle)
	{
		pAngle->x = m_angleH;
		pAngle->y = m_angleV;
	}
}

void _VisionBase::setAttitude(double rollRad, double pitchRad, uint64_t timestamp)
{
	Point2f center(m_centerH, m_centerV);
	double deg = -rollRad * 180.0 * OneOvPI;

	m_rotRoll = getRotationMatrix2D(center, deg, m_isoScale);
	//TODO: add rot estimation

}

Mat* _VisionBase::K(void)
{
	return &m_K;
}

VISION_TYPE _VisionBase::getType(void)
{
	return m_type;
}

bool _VisionBase::isOpened(void)
{
	return m_bOpen;
}


}
