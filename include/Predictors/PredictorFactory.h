/**
 *  ==============================================================================
 *
 *          \file   Predictors/PredictorFactory.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-10 08:04:52.394292
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PREDICTORS__PREDICTOR_FACTORY_H_
#define PREDICTORS__PREDICTOR_FACTORY_H_

#include "Predictors/LinearPredictor.h"
namespace gezi {

class PredictorFactory 
{
public:
	static PredictorPtr CreatePredictor(string name)
	{
		boost::to_lower(name);
		if (name == "linearsvm" || name == "linear" || name == "linearpredictor")
		{
			return make_shared<LinearPredictor>();
		}
		LOG(WARNING) << name << " is not supported now, return nullptr";
		return nullptr;
	}

	static PredictorPtr CreatePredictor(string name, string path)
	{
		PredictorPtr predictor = CreatePredictor(name);
		if (predictor != nullptr)
		{
			predictor->Load(path);
		}
		return predictor;
	}

	static PredictorPtr LoadPredictor(string path)
	{
		string name = read_file(path + "/model.name.txt");
		PredictorPtr predictor = CreatePredictor(name);
		if (predictor != nullptr)
		{
			predictor->Load(path);
		}
		return predictor;
	}

protected:
private:

};

}  //----end of namespace gezi

#endif  //----end of PREDICTORS__PREDICTOR_FACTORY_H_