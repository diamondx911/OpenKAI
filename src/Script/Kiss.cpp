#include "Kiss.h"

namespace kai
{

Kiss::Kiss(void)
{
	m_nChild = 0;
	m_class = "";
	m_name = "";
	m_bNULL = false;
	m_pNULL = NULL;
	m_pParent = NULL;
	m_pInst = NULL;
}

Kiss::~Kiss(void)
{
	for(int i=0;i<m_nChild;i++)
	{
		DEL(m_pChild[i]);
	}
}

bool Kiss::parse(string* pStr)
{
	if (m_bNULL)return false;

	//Create NULL instance
	if(m_pParent)
	{
		m_pNULL = this->m_pParent->m_pNULL;
	}
	else if (!m_pNULL)
	{
		m_pNULL = new Kiss();
		m_pNULL->m_bNULL = true;
	}

	int k;
	std::string::size_type from, to;

	trim(pStr);
	delComment(pStr);

	do
	{
		//find the object start
		from = pStr->find('{');
		if (from == std::string::npos)
			break;

		//find the paired bracket
		to = from + 1;
		k = 0;
		while (to < pStr->length())
		{
			if ((*pStr)[to] == '{')
			{
				k++;
			}
			else if ((*pStr)[to] == '}')
			{
				if (k == 0)
					break;
				k--;
			}
			to++;
		}

		//the pair bracket not found
		if (to == pStr->length())
			return false;

		//check if it is a null object
		if (to > from + 1)
		{
			//create new obj
			string subStr = pStr->substr(from + 1, to - from - 1);
			addChild(&subStr);
		}

		pStr->erase(from, to - from + 1);

	} while (1);

	if (!m_json.parse("{" + (*pStr) + "}"))
		return false;

	string name;
	name = "name";
	m_json.v(&name, &m_name);

	name = "class";
	m_json.v(&name, &m_class);

	bool bInst = true;
	name = "bInst";
	m_json.v(&name, &bInst);

	return bInst;
}

void Kiss::trim(string* pStr)
{
	std::string::size_type k;

	//do NOT delete white spaces as gst pipeline use it for parameter separations

	k = pStr->find('\r');
	while (k != std::string::npos)
	{
		pStr->erase(k,1);
		k = pStr->find('\r');
	}

	k = pStr->find('\t');
	while (k != std::string::npos)
	{
		pStr->erase(k,1);
		k = pStr->find('\t');
	}
}

void Kiss::delComment(string* pStr)
{
	std::string::size_type cFrom;
	std::string::size_type cTo;
	string commentFrom = "/*";
	string commentTo = "*/";

	cFrom = pStr->find(commentFrom);
	while (cFrom != std::string::npos)
	{
		cTo = pStr->find(commentTo, cFrom + commentFrom.length());
		if(cTo == std::string::npos)
		{
			cTo = pStr->length() - commentTo.length();
		}

		pStr->erase(cFrom, cTo - cFrom + commentTo.length());
		cFrom = pStr->find(commentFrom);
	}
}

bool Kiss::addChild(string* pStr)
{
	if (m_bNULL)
		return false;
	if (m_nChild >= NUM_CHILDREN)
		return false;

	Kiss* pChild = new Kiss();
	pChild->m_pParent = this;
	if(!pChild->parse(pStr))
	{
		delete pChild;
		return false;
	}

	m_pChild[m_nChild] = pChild;
	m_nChild++;

	return true;
}

Kiss* Kiss::o(string name)
{
	if (name.empty())
		return m_pNULL;
	if (m_bNULL)
		return m_pNULL;

	for (int i = 0; i < m_nChild; i++)
	{
		if (m_pChild[i]->m_name == name)
			return m_pChild[i];
	}

	return m_pNULL;
}

Kiss* Kiss::root(void)
{
	Kiss* pRoot = this;

	while(pRoot->m_pParent)
	{
		pRoot = pRoot->m_pParent;
	}

	return pRoot;
}

Kiss* Kiss::parent(void)
{
	return m_pParent;
}

JSON* Kiss::json(void)
{
	return &m_json;
}

Kiss** Kiss::getClassItr(string* pClassName)
{
	if(pClassName==NULL)return NULL;
	if(pClassName->empty())return NULL;

	int i;
	int nFound = 0;

	//Find list with the class name
	for(i=0; i<m_nChild; i++)
	{
		Kiss* pC = m_pChild[i];
		if(pC->m_class != (*pClassName))continue;

		m_ppItr[nFound]=pC;
		nFound++;

		if(nFound == NUM_CHILDREN-1)break;
	}

	m_ppItr[nFound]=NULL;
	nFound++;

	return m_ppItr;
}

Kiss** Kiss::getChildItr(void)
{
	m_pChild[m_nChild]=NULL;

	return m_pChild;
}

void* Kiss::getChildInstByName(string* pName)
{
	NULL_N(pName);
	Kiss* pC = o(*pName);
	IF_N(!pC);
	IF_N(pC->empty());

	return pC->m_pInst;
}

bool Kiss::empty(void)
{
	return m_bNULL;
}

bool Kiss::v(string name, int* val)
{
	return m_json.v(&name, val);
}

bool Kiss::v(string name, bool* val)
{
	return m_json.v(&name, val);
}

bool Kiss::v(string name, uint64_t* val)
{
	return m_json.v(&name, val);
}

bool Kiss::v(string name, double* val)
{
	return m_json.v(&name, val);
}

bool Kiss::v(string name, string* val)
{
	return m_json.v(&name, val);
}

int Kiss::array(string name, int* pVal, int nArray)
{
	return m_json.array(&name, pVal, nArray);
}

int Kiss::array(string name, double* pVal, int nArray)
{
	return m_json.array(&name, pVal, nArray);
}

int Kiss::array(string name, string* pVal, int nArray)
{
	return m_json.array(&name, pVal, nArray);
}

}
