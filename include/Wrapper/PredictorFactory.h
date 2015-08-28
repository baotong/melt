/**
 *  ==============================================================================
 *
 *          \file   Wrapper/PredictorFactory.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-15 11:51:55.436143
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef WRAPPER__PREDICTOR_FACTORY_H_
#define WRAPPER__PREDICTOR_FACTORY_H_

//#include "conf_util.h"  //@TODO  �������common_util.h ����conf_util.h�����Max��ͻ ��Ȼ������ullog sb ����why..
#include "common_util.h" 
#include "MLCore/PredictorFactory.h"

DECLARE_string(model_dir);
namespace gezi {
	namespace wrapper {
		//�����·������shared conf ������
		class PredictorFactory
		{
		public:
			//@FIXME ����������л����� ���������cpp�ļ�ʵ�־ͻ���֡�����
			static Predictors LoadPredictors()
			{
				Predictors predictors;
				if (!FLAGS_model_dir.empty())
				{
					predictors.push_back(gezi::PredictorFactory::LoadPredictor(FLAGS_model_dir));
					return predictors;
				}
				string section = "Predictor";
				int model_cnt = 1;
				SCONF(model_cnt);
				for (int i = 0; i < model_cnt; i++)
				{
					string name = format("model_{}_dir", i);
					string modelDir;
					gezi::set_val(gezi::SharedConf::conf(), section, gezi::conf_trim(name), modelDir);
					predictors.push_back(gezi::PredictorFactory::LoadPredictor(modelDir));
				}
				return predictors;
			}
		protected:
		private:

		};

	}  //----end of namespace wrapper
}  //----end of namespace gezi

#endif  //----end of WRAPPER__PREDICTOR_FACTORY_H_
