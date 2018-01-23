#include "Startup.h"

Startup* g_pStartup;
void onMouseGeneral(int event, int x, int y, int flags, void* userdata)
{
	g_pStartup->handleMouse(event, x, y, flags);
}

void signalHandler(int signal)
{
	if (signal != SIGINT)return;
	printf("\nSIGINT: OpenKAI Shutdown\n");
	g_pStartup->m_bRun = false;
}

namespace kai
{

Startup::Startup()
{
	for (int i = 0; i < N_INST; i++)
	{
		m_pInst[i] = NULL;
		m_pMouse[i] = NULL;
	}
	m_nInst = 0;
	m_nMouse = 0;

	m_appName = "";
	m_bWindow = true;
	m_waitKey = 50;
	m_bRun = true;
	m_key = 0;
	m_bLog = true;
	m_winMouse = "";
}

Startup::~Startup()
{
}

string* Startup::getName(void)
{
	return &m_appName;
}

bool Startup::start(Kiss* pKiss)
{
	g_pStartup = this;
	signal(SIGINT, signalHandler);

	NULL_F(pKiss);
	Kiss* pApp = pKiss->root()->o("APP");
	IF_F(pApp->empty());

	KISSm(pApp,appName);
	KISSm(pApp,bWindow);
	KISSm(pApp,waitKey);
	KISSm(pApp,winMouse);

	//create instances
	F_FATAL_F(createAllInst(pKiss));

	//link instances with each other
	int i;
	for (i = 0; i < m_nInst; i++)
	{
		F_FATAL_F(m_pInst[i]->link());
	}

	if(m_winMouse != "" && m_bWindow)
	{
		setMouseCallback(m_winMouse, onMouseGeneral, 0 );
	}

	//UI thread
	m_bRun = true;

	if (m_bWindow)
	{
		while (m_bRun)
		{
			draw();
			m_key = waitKey(m_waitKey);
			handleKey(m_key);
		}
	}
	else
	{
		while (m_bRun)
		{
			sleep(1);
		}
	}

	for (i = 0; i < m_nInst; i++)
	{
		m_pInst[i]->reset();
	}

	for (i = 0; i < m_nInst; i++)
	{
		DEL(m_pInst[i]);
	}

	return 0;
}

void Startup::draw(void)
{
	for (int i = 0; i < m_nInst; i++)
	{
		BASE* pInst = m_pInst[i];
		string* cName = pInst->getClass();

		pInst->draw();
	}
}

void Startup::handleKey(int key)
{
	switch (key)
	{
	case 27:
		m_bRun = false;	//ESC
		break;
	default:
		break;
	}
}

void Startup::handleMouse(int event, int x, int y, int flags)
{
	MOUSE m;
	m.m_event = event;
	m.m_flags = flags;
	m.m_x = x;
	m.m_y = y;

	int i;
	for (i = 0; i < m_nMouse; i++)
	{
		((_AutoPilot*) m_pMouse[i])->onMouse(&m);
	}
}

bool Startup::createAllInst(Kiss* pKiss)
{
	NULL_F(pKiss);
	Kiss** pItr = pKiss->root()->getChildItr();

	int i = 0;
	while (pItr[i])
	{
		Kiss* pK = pItr[i++];
		if (pK->m_class == "Startup")
			continue;

		BASE* pNew = m_module.createInstance(pK);
		if (pNew == NULL)
		{
			LOG_E("Create instance failed: " + pK->m_name);
			continue;
		}

		if (m_nInst >= N_INST)
		{
			LOG_E("Number of module instances reached limit");
			return false;
		}

		m_pInst[m_nInst] = pNew;
		m_nInst++;

		F_FATAL_F(pNew->start());

		if (*pNew->getClass() != "_AutoPilot")
			continue;
		if (!m_bWindow)
			continue;

		m_pMouse[m_nMouse] = pNew;
		m_nMouse++;
	}

	return true;
}

}
