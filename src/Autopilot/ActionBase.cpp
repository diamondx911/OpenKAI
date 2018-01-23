#include "ActionBase.h"

namespace kai
{

ActionBase::ActionBase()
{
	m_pAM = NULL;
	m_timeStamp = 0;
	m_dTime = 0;
	m_vActiveState.clear();
	m_iLastState = 0;
	m_bStateChanged = false;
}

ActionBase::~ActionBase()
{
}

bool ActionBase::init(void* pKiss)
{
	IF_F(!this->BASE::init(pKiss));

	m_vActiveState.clear();
	return true;
}

bool ActionBase::link(void)
{
	IF_F(!this->BASE::link());
	Kiss* pK = (Kiss*)m_pKiss;

	string iName="";
	F_INFO(pK->v("_Automaton", &iName));
	m_pAM = (_Automaton*) (pK->root()->getChildInstByName(&iName));
	NULL_T(m_pAM);

	m_vActiveState.clear();
	string pAS[N_ACTIVESTATE];
	int nAS = pK->array("activeState", pAS, N_ACTIVESTATE);
	for(int i=0; i<nAS; i++)
	{
		int iState = m_pAM->getStateIdx(&pAS[i]);
		if(iState<0)continue;

		m_vActiveState.push_back(iState);
	}

	m_iLastState = m_pAM->getCurrentStateIdx();

	return true;
}

void ActionBase::update(void)
{
	uint64_t newTime = getTimeUsec();
	m_dTime = newTime - m_timeStamp;
	m_timeStamp = newTime;

	NULL_(m_pAM);
	int currentState = m_pAM->getCurrentStateIdx();
	if(m_iLastState != currentState)
	{
		m_bStateChanged = true;
		m_iLastState = currentState;
	}
	else
	{
		m_bStateChanged = false;
	}
}

bool ActionBase::isActive(void)
{
	NULL_F(m_pAM);

	int iState = m_pAM->getCurrentStateIdx();
	for (int i : m_vActiveState)
	{
		if(iState == i)return true;
	}

	return false;
}

bool ActionBase::isStateChanged(void)
{
	return m_bStateChanged;
}

bool ActionBase::draw(void)
{
	return this->BASE::draw();
}


}
