/*
 * _ImageNet.h
 *
 *  Created on: Aug 17, 2016
 *      Author: yankai
 */

#ifndef OpenKAI_src_DNN_Classifier__ImageNet_H_
#define OpenKAI_src_DNN_Classifier__ImageNet_H_

#include "../../Base/common.h"
#include "../../Detector/_DetectorBase.h"

#ifdef USE_TENSORRT

namespace kai
{

class _ImageNet: public _DetectorBase
{
public:
	_ImageNet();
	~_ImageNet();

	bool init(void* pKiss);
	bool link(void);
	bool start(void);
	bool draw(void);
	void reset(void);
	int getClassIdx(string& className);
	string getClassName(int iClass);

	int classify(Frame* pBGR, string* pName);
	bool bReady(void);

private:
	void detect(void);
	void classifyBatch(int iObj, int nBatch);
	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_ImageNet*) This)->update();
		return NULL;
	}

public:
	imageNet* m_pIN;
	Frame* m_pRGBA;

	int		m_nBatch;
	string	m_blobIn;
	string	m_blobOut;
	int		m_maxPix;

	uint64_t* m_pmClass;
	int* m_piClass;

};

}

#endif
#endif
