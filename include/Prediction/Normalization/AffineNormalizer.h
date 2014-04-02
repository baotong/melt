/**
 *  ==============================================================================
 *
 *          \file   Prediction/Normalization/AffineNormalizer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-01 17:07:57.638996
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PREDICTION__NORMALIZATION__AFFINE_NORMALIZER_H_
#define PREDICTION__NORMALIZATION__AFFINE_NORMALIZER_H_
#include "Prediction/Normalization/Normalizer.h"
namespace gezi {

	class AffineNormalizer : public Normalizer
	{
	public:
		AffineNormalizer()
		{
			_func = [this](int index, Float& value)
			{
				/*	if (_scales[index] <= 0)
					{
					value = 0;
					}
					else if (_trunct)*/
				if (_trunct)
				{
					if (value >= _offsets[index] + _scales[index])
						value = _upper;
					else if (value <= _offsets[index])
						value = _lower;
					else
						value = _lower + _range * (value - _offsets[index]) / _scales[index];
				}
				else
				{
					value = _lower + _range * (value - _offsets[index]) / _scales[index];
				}
			};
		}

		virtual void Begin() override
		{
			_offsets.resize(_featureNum, 0);
			_scales.resize(_featureNum, 0);
		}

		virtual void Finalize() override
		{

		}

		void CheckOffsetScale()
		{
			for (int i = 0; i < _numFeatures; i++)
			{
				if (_scales[i] <= 0)
				{ //����TLC �����ʼ��ֵһ�� ��Ȼά��ԭ�� ��scale ����Ϊ0 @TODO
					//��Ч���� ʼ����6�ı��� ����6 �����0 @TODO ��Ҫ��Ϊ0��
					LOG(WARNING) << "Feature " << i << " : " << _featureNames[i]
						<< " always take value " << _offsets[i];
				}
				else if (_offsets[i] != _lower || _scales[i] != _upper)
				{ //like [0,1] range will not need to transform
					_scaleIndices.push_back(i);
				}
				else if (_trunct)
				{ //���Ҫ�ضϼ�ʹ[0,1]Ҳ��Ҫscale���ܣ� ע��ֻ���ܻ������߲�����trunct ����test���֣�@TODO only test trunct ?
					_scaleIndices.push_back(i);
				}
			}
		}

		virtual void NormalizeCore(Vector& vec) override
		{
			Normalize(vec, _func);
		}

	protected:
		Fvec _offsets;
		Fvec _scales;
	private:
		std::function<Float(int, Float)> _func;
	};

}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__AFFINE_NORMALIZER_H_
