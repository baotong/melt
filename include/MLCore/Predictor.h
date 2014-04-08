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

//当前只是考虑 二分类模型
//@TODO 多分类模型支持 重载 Fvec 或者Predict返回Fvec ?, Regression Rank等模型的支持
//@TODO TLC c#的那种接口的设计 能够迁移到c++吗
//var stronglyTypedPredictor = predictor as IPredictor<Instance, float>;
class Predictor 
{
public:
	//输出未经Calibrator矫正的数值 -n,+n 0表示分界 越高越倾向positive
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

	//输出经Calibrator矫正的数值 [0,1], 输出值表示结果倾向positive的概率
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
