/*
 *  Created on: Nov 4, 2016
 *      Author: yankai
 */
#include "_Lightware_SF40.h"

namespace kai
{

_Lightware_SF40::_Lightware_SF40()
{
	m_pIn = NULL;
	m_pOut = NULL;

	m_hdg = 0.0;
	m_offsetAngle = 0.0;
	m_nDiv = 0;
	m_dAngle = 0;
	m_minDist = 1.0;
	m_maxDist = 50.0;
	m_showScale = 1.0;	//1m = 1pixel;
	m_strRecv = "";
	m_pSF40sender = NULL;
	m_MBS = 0;

	m_pDist = NULL;
	m_medianFilter = 3;

	m_dPos.init();
	m_lastPos.init();

}

_Lightware_SF40::~_Lightware_SF40()
{
	DEL(m_pSF40sender);

	if (m_pIn)
	{
		m_pIn->close();
		delete m_pIn;
		m_pIn = NULL;
	}

	if (m_pOut)
	{
		m_pOut->close();
		delete m_pOut;
		m_pOut = NULL;
	}

	DEL(m_pDist);
}

bool _Lightware_SF40::init(void* pKiss)
{
	CHECK_F(!this->_ThreadBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;
	pK->m_pInst = this;

	string presetDir = "";
	F_INFO(pK->root()->o("APP")->v("presetDir", &presetDir));

	//common in all input modes
	F_INFO(pK->v("offsetAngle", &m_offsetAngle));
	F_INFO(pK->v("minDist", &m_minDist));
	F_INFO(pK->v("maxDist", &m_maxDist));
	F_INFO(pK->v("showScale", &m_showScale));
	F_INFO(pK->v("MBS", (int* )&m_MBS));

	F_ERROR_F(pK->v("nDiv", &m_nDiv));
	m_dAngle = DEG_AROUND / m_nDiv;

	F_INFO(pK->v("medianFilter", &m_medianFilter));
	m_pDist = new Filter[m_nDiv];
	for(int i=0;i<m_nDiv;i++)
	{
		m_pDist->startMedian(m_medianFilter);
	}

	//IO
	Kiss* pCC;
	string param = "";

	//input
	pCC = pK->o("input");
	CHECK_F(pCC->empty());
	F_ERROR_F(pCC->v("class", &param));
	if (param == "SerialPort")
	{
		m_pIn = new SerialPort();
		CHECK_F(!m_pIn->init(pCC));

		m_pSF40sender = new _Lightware_SF40_sender();
		m_pSF40sender->m_pSerialPort = (SerialPort*) m_pIn;
		m_pSF40sender->m_dAngle = m_dAngle;
		CHECK_F(!m_pSF40sender->init(pKiss));
	}
	else if (param == "File")
	{
		m_pIn = new File();
		F_ERROR_F(m_pIn->init(pCC));
	}
	else if (param == "TCP")
	{
		m_pIn = new TCP();
		F_ERROR_F(m_pIn->init(pCC));
	}

	//output
	pCC = pK->o("output");
	CHECK_T(pCC->empty());
	param = "";
	F_ERROR_F(pCC->v("class", &param));
	if (param == "SerialPort")
	{
		m_pOut = new SerialPort();
		CHECK_F(m_pOut->init(pCC));
	}
	else if (param == "File")
	{
		m_pOut = new File();
		F_ERROR_F(m_pOut->init(pCC));
	}
	else if (param == "TCP")
	{
		m_pOut = new TCP();
		F_ERROR_F(m_pOut->init(pCC));
	}

	return true;
}

bool _Lightware_SF40::link(void)
{
	CHECK_F(!this->_ThreadBase::link());
	Kiss* pK = (Kiss*) m_pKiss;

	return true;
}

bool _Lightware_SF40::start(void)
{
	if (m_pIn->type() == serialport)
	{
		CHECK_F(!m_pSF40sender->start());
	}

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

void _Lightware_SF40::update(void)
{
	while (m_bThreadON)
	{
		if (!m_pIn->isOpen())
		{
			if (!m_pIn->open())
			{
				this->sleepTime(USEC_1SEC);
				continue;
			}

			if (m_pIn->type() == serialport)
				m_pSF40sender->MBS(m_MBS);
		}

		this->autoFPSfrom();

		if(updateLidar())
		{
			updatePosition();
		}

		this->autoFPSto();
	}

}

bool _Lightware_SF40::updateLidar(void)
{
	CHECK_F(!readLine());

	string str;
	istringstream sStr;

	//separate the command part
	vector<string> vStr;
	vStr.clear();
	sStr.str(m_strRecv);
	while (getline(sStr, str, ' '))
	{
		vStr.push_back(str);
	}
	if(vStr.size() < 2)
	{
		m_strRecv.clear();
		return false;
	}

	string cmd = vStr.at(0);
	string result = vStr.at(1);

	//find the angle from cmd
	vector<string> vCmd;
	sStr.clear();
	sStr.str(cmd);
	while (getline(sStr, str, ','))
	{
		vCmd.push_back(str);
	}
	if(vCmd.size() < 2)
	{
		m_strRecv.clear();
		return false;
	}
	double angle = atof(vCmd.at(1).c_str());

	//find the result
	vector<string> vResult;
	sStr.clear();
	sStr.str(result);
	while (getline(sStr, str, ','))
	{
		vResult.push_back(str);
	}
	if(vResult.size() < 1)
	{
		m_strRecv.clear();
		return false;
	}

	double dist = atof(vResult.at(0).c_str());

	angle += m_hdg + m_offsetAngle;
	while(angle >= DEG_AROUND)
		angle -= DEG_AROUND;

	int iAngle = (int) (angle / m_dAngle);
	if(iAngle >= m_nDiv)
	{
		m_strRecv.clear();
		return false;
	}

	m_pDist[iAngle].input(dist);
	m_strRecv.clear();

	return true;
}

bool _Lightware_SF40::readLine(void)
{
	char buf;
	while (m_pIn->read((uint8_t*) &buf, 1) > 0)
	{
		if (buf == 0)
			continue;
		if (buf == LF || buf == CR)
		{
			if(m_strRecv.empty())continue;
			CHECK_T(m_pOut==NULL);

			if (!m_pOut->isOpen())
			{
				CHECK_T(!m_pOut->open());
			}

			m_pOut->writeLine((uint8_t*) m_strRecv.c_str(), m_strRecv.length());
			return true;
		}

		m_strRecv += buf;
	}

	return false;
}

void _Lightware_SF40::updatePosition(void)
{
	int i;
	double nV;
	double pX = 0;
	double pY = 0;

	for (i = 0, nV = 0.0; i < m_nDiv; i++)
	{
		double dist = m_pDist[i].v();
		if (dist < m_minDist)
			continue;
		if (dist > m_maxDist)
			continue;

		//TODO: check the variance of the angle, reset if beyond thr

		double angle = (m_dAngle * i) * DEG_RADIAN;
		pX += (dist * sin(angle));
		pY += -(dist * cos(angle));
		nV += 1.0;
	}

	nV = 1.0/nV;
	pX *= nV;
	pY *= nV;
//	m_pX->input(pX);
//	m_pY->input(pY);

	vDouble2 newPos;
//	newPos.m_x = m_pX->v();
//	newPos.m_y = m_pY->v();
	m_dPos.m_x = newPos.m_x - m_lastPos.m_x;
	m_dPos.m_y = newPos.m_y - m_lastPos.m_y;
	m_lastPos = newPos;
}

void _Lightware_SF40::setHeading(double hdg)
{
	m_hdg = hdg;
}

vDouble2* _Lightware_SF40::getDiffPos(void)
{
	return &m_dPos;
}

void _Lightware_SF40::reset(void)
{
	for(int i=0;i<m_nDiv;i++)
	{
		m_pDist->reset();
	}
	m_dPos.init();
	m_lastPos.init();
}

bool _Lightware_SF40::draw(void)
{
	CHECK_F(!this->_ThreadBase::draw());
	Window* pWin = (Window*)this->m_pWindow;
	Mat* pMat = pWin->getFrame()->getCMat();
	string msg;

//	pWin->tabNext();
//	msg = "Pos: X=" + f2str(m_pX->v()) + ", Y=" + f2str(m_pY->v());
//	pWin->addMsg(&msg);

	if (m_pIn)
		m_pIn->draw();

	if (m_pOut)
		m_pOut->draw();

	pWin->tabPrev();
	CHECK_T(m_nDiv <= 0);

	//Plot center as vehicle position
	Point pCenter(pMat->cols/2, pMat->rows/2);
	circle(*pMat, pCenter, 10, Scalar(0, 0, 255), 2);

	//Plot lidar result
	for (int i = 0; i < m_nDiv; i++)
	{
		double dist = m_pDist[i].v() * m_showScale;
		double angle = m_dAngle * i * DEG_RADIAN;
		int pX = (dist * sin(angle));
		int pY = -(dist * cos(angle));

		circle(*pMat, pCenter+Point(pX, pY), 1, Scalar(0, 255, 0), 1);
	}

	return true;
}

}
