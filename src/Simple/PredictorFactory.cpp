/**
 *  ==============================================================================
 *
 *          \file   PredictorFactory.cpp
 *
 *        \author   chenghuige
 *
 *          \date   2014-07-26 09:34:14.603965
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PREDICTOR_FACTORY_CPP_
#define PREDICTOR_FACTORY_CPP_

#include "Simple/PredictorFactory.h"
#include "MLCore/PredictorFactory.h"
#include "MLCore/Predictor.h"
namespace gezi {
	namespace simple {
		vector<PredictorPtr> _predictors;
		Predictor PredictorFactory::LoadPredictor(std::string path)
		{
			_predictors.push_back(gezi::PredictorFactory::LoadPredictor(path));
			Predictor predictor;
			predictor._predictor = _predictors.back().get();
			return predictor;
		} //@FIXME Ϊʲô��������Ҫvector��holdָ��,���ֱ���ڲ�һ��shared ptrΪ�����ص�predictor������ڲ�_predictor��Ч �Ѿ����ͷ����� ����ò��ֱ������������õĻ�ok,�ο�gezi test�����test_shared_ptr.cc�����÷�,��˵�Ǹ�Ҳ������ָͨ��ȥ�ӵ�.get
	}
}  //----end of namespace gezi

#endif
