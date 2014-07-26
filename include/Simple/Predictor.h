/**
 *  ==============================================================================
 *
 *          \file   Simple/Predictor.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-07-26 09:32:30.332895
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef SIMPLE__PREDICTOR_H_
#define SIMPLE__PREDICTOR_H_

#include <vector>
#include <utility>
namespace gezi {
	namespace simple {
		/*ע�����붼����const,�����vector�ᱻ�ı�*/
		class Predictor
		{
		public:

			//----------------�����������ܱ�ʾ,�Ƽ�,һ���ٶȻ����(��gbdtԤ��)
			//����ع�ʵ��ֵ
			double Output(std::vector<double>& values);
			//���[0-1]��Χ�ĸ���ֵ
			double Predict(std::vector<double>& values);
			//output���ػع�ʵ��ֵ����������[0-1]����ֵ
			double Predict(std::vector<double>& values, double& output);

			//----------------�����������ܱ�ʾ
			double Output(std::vector<int>& indices, std::vector<double>& values);
			double Predict(std::vector<int>& indices, std::vector<double>& values);
			double Predict(std::vector<int>& indices, std::vector<double>& values, double& output);

		private:
			void* _predictor;
		};
	}
}  //----end of namespace gezi

#endif  //----end of SIMPLE__PREDICTOR_H_
