
#ifndef OpenKAI_src_Autopilot_Controller_APcopter_visualFollow_H_
#define OpenKAI_src_Autopilot_Controller_APcopter_visualFollow_H_

#include "../../../Base/common.h"
#include "../../../DNN/Detector/_ClusterNet.h"
#include "../../ActionBase.h"
#include "APcopter_base.h"


namespace kai
{

class APcopter_visualFollow: public ActionBase
{
public:
	APcopter_visualFollow();
	~APcopter_visualFollow();

	bool init(void* pKiss);
	bool link(void);
	void update(void);
	bool draw(void);

private:
	APcopter_base* m_pAP;
	_ClusterNet*	m_pMN;

};

}

#endif

