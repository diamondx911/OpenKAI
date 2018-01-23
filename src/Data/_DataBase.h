/*
 * _DataBase.h
 *
 *  Created on: Oct 16, 2017
 *      Author: yankai
 */

#ifndef OpenKAI_src_Data__DataBase_H_
#define OpenKAI_src_Data__DataBase_H_

#include "../Base/common.h"
#include "../Base/_ThreadBase.h"
#include "../Utility/util.h"

#define N_EXT 16
#define D_TYPE_FILE 0x8
#define D_TYPE_FOLDER 0x4

namespace kai
{

class _DataBase: public _ThreadBase
{
public:
	_DataBase();
	~_DataBase();

	bool init(void* pKiss);
	bool link(void);
	bool start(void);
	void reset(void);

	int getDirFileList(void);
	bool verifyExtension(string* fName);

private:
	void getDirFileList(string* pStrDir);
	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_DataBase*) This)->update();
		return NULL;
	}

public:
	string m_dirIn;
	vector<string> m_vExtIn;
	string m_extOut;

	vector<string> m_vFileIn;
	vector<int> m_PNGcompress;
	int m_PNGcompression;


};
}

#endif
