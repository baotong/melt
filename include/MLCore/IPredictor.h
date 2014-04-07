/**
 *  ==============================================================================
 *
 *          \file   IPredictor.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-02-03 18:04:14.599408
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef IPREDICTOR_H_
#define IPREDICTOR_H_

#include "common_util.h"
#include "Prediction/Instances/Instance.h"
namespace gezi {

	//���ڿ�������Feature���޸� ������ʱ��֧������Ԥ��
	class IPredictor
	{
	public:
		//���δ��Calibrator��������ֵ -n,+n 0��ʾ�ֽ� Խ��Խ����postive
		Float Output(const Instance& instance)
		{
			return Output(instance.features);
		}
		Float Output(InstancePtr instance)
		{
			return Output(instance->features);
		}
		virtual Float Output(const Vector& features) = 0;
	};

	class IDistributionPredictor 
	{
	public:
		//�����Calibrator��������ֵ [0,1]
		Float Predict(const Instance& instance)
		{
			return Predict(instance.features);
		}
		Float Predict(InstancePtr instance)
		{
			return Predict(instance->features);
		}
		virtual Float Predict(const Vector& features) = 0;
	};
}  //----end of namespace gezi

#endif  //----end of IPREDICTOR_H_
