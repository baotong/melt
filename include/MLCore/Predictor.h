/**
 *  ==============================================================================
 *
 *          \file   MLCore/Predictor.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-07 22:07:30.510777
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef M_L_CORE__PREDICTOR_H_
#define M_L_CORE__PREDICTOR_H_

#include "common_util.h"
#include "Prediction/Instances/Instance.h"
namespace gezi {

//��ǰֻ�ǿ��� ������ģ��
//@TODO �����ģ��֧�� ���� Fvec ����Predict����Fvec ?, Regression Rank��ģ�͵�֧��
//@TODO TLC c#�����ֽӿڵ���� �ܹ�Ǩ�Ƶ�c++��
//var stronglyTypedPredictor = predictor as IPredictor<Instance, float>;
class Predictor 
{
public:
	//���δ��Calibrator��������ֵ -n,+n 0��ʾ�ֽ� Խ��Խ����positive
	Float Output(Instance& instance)
	{
		return Output(instance.features);
	}
	Float Output(InstancePtr instance)
	{
		return Output(instance->features);
	}
	
	virtual Float Output(Vector& features)
	{
		return 0;
	}

	//�����Calibrator��������ֵ [0,1], ���ֵ��ʾ�������positive�ĸ���
	Float Predict(Instance& instance)
	{
		return Predict(instance.features);
	}
	Float Predict(InstancePtr instance)
	{
		return Predict(instance->features);
	}

	Float Predict(Vector& features)
	{
		return Predict(Output(features));
	}

	Float Predict(Instance& instance, Float& output)
	{
		return Predict(instance.features, output);
	}

	Float Predict(InstancePtr instance, Float& output)
	{
		return Predict(instance->features, output);
	}

	Float Predict(Vector& features, Float& output)
	{
		output = Output(features);
		return Predict(output);
	}
protected:
	virtual Float Predict(Float output)
	{

	}
private:

};

typedef shared_ptr<Predictor> PredictorPtr;
}  //----end of namespace gezi

#endif  //----end of M_L_CORE__PREDICTOR_H_
