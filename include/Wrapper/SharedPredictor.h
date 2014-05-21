/**
 *  ==============================================================================
 *
 *          \file   Wrapper/SharedPredictor.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-15 12:51:20.935057
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef WRAPPER__SHARED_PREDICTOR_H_
#define WRAPPER__SHARED_PREDICTOR_H_


#include "Wrapper/PredictorFactory.h"
#include "Wrapper/SharedPredictors.h"
namespace gezi {

	//֧�������Ͷ���ģʽ
	class SharedPredictor
	{
	public:
		static PredictorPtr& Instance()
		{
			return SharedPredictors::Instance();
		}

		static void Init()
		{
			Instance();
		}
	};
}  //----end of namespace gezi

#endif  //----end of WRAPPER__SHARED_PREDICTOR_H_
