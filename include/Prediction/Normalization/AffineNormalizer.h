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
				if (_scales[index] <= 0)
				{//���ֲ��� Dense Sparse �����߼�һ�� �����Ϊ0 
					//��ҪCheckOffsetAndScale������shiftIndices����scale <=0 ��offset!=0�ĵ�
					//value = 0;
					return;
				}
				else if (_trunct)
				{
					if (value >= _offsets[index] + _scales[index])
						value = 1.0;
					else if (value <= _offsets[index])
						value = 0.0;
					else
						value = (value - _offsets[index]) / _scales[index];
				}
				else
				{
					value = (value - _offsets[index]) / _scales[index];
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

		void CheckOffsetAndScale()
		{
			for (int i = 0; i < _featureNum; i++)
			{
				if (_scales[i] <= 0)
				{ //����TLC �����ʼ��ֵһ�� ��Ȼά��ԭ�� ��scale ����Ϊ0 @TODO
					//��Ч���� ʼ����6�ı��� ����6 �����0 @TODO ��Ҫ��Ϊ0��
					VLOG(4) << "Feature " << i << " : " << _featureNames[i]
						<< " always take value " << _offsets[i];
				}
				else if (_offsets[i] != 0.0)
				{ //�����Сֵ��0 ��ô ���еĵ㶼ֻ��Ҫmorph����Ҫshift��ͬʱ�����0ֵ�� ����Ҫ�仯,dense ʹ��morphIndices, sparseʹ��shiftIdices �����󲢼� ��0 union ��Ҫmoph��0ֵ��
					_shiftIndices.push_back(i);
					_morphIndices.push_back(i);
				}
				else if (_scales[i] != 1.0)
				{
					_morphIndices.push_back(i);
				} //like [0,1] ���������е㶼����Ҫ�仯
				else if (_trunct)
				{ //���Ҫ�ضϼ�ʹ[0,1]Ҳ��Ҫscale���ܣ� ע��ֻ���ܻ������߲�����trunct ����test���֣�@TODO only test trunct ?
					_morphIndices.push_back(i);
				}
			}

			PVEC(_offsets);
			PVEC(_scales);
			PVEC(_morphIndices);
			PVEC(_shiftIndices);
		}

		virtual void NormalizeCore(Vector& vec) override
		{
			Normalize(vec, _func);
		}

	protected:
		Fvec _offsets;
		Fvec _scales;
	private:
		std::function<void(int, Float&)> _func;
	};

}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__AFFINE_NORMALIZER_H_
