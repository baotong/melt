/**
 *  ==============================================================================
 *
 *          \file   Prediction/Normalization/MinMaxNormalizer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-02 10:30:54.515916
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PREDICTION__NORMALIZATION__MIN_MAX_NORMALIZER_H_
#define PREDICTION__NORMALIZATION__MIN_MAX_NORMALIZER_H_

#include "Prediction/Normalization/AffineNormalizer.h"
namespace gezi {

	class MinMaxNormalizer : public AffineNormalizer
	{
	public:
		virtual void Process(const Vector& vec)
		{
			if (_isFirst)
			{
				_counts.resize(_featureNum, 1);
				vec.ForEachAll([this](int i, Float value)
				{
					_offsets[i] = value;
					_scales[i] = value;
				});
				_isFirst = false;
			}
			else
			{ //set lower and upper bound for each i
				///������ֻ�б��� -13 0  0  0 ���涼�� 0 ���߶��� -5 -4 -3 ���� ��ô���ֵû��ȡ��ʵ�ʵ� 0
				//���߱��� 3 4 5 ... ��Сֵû��ȡ��ʵ�ʵ� 0  ò��TLC����ط������� û�����⴦��
				vec.ForEach([this](int i, Float value)
				{
					_counts[i]++;
					this->SetOffsetScale(i, value); //need this->
				});
				_total++;
			}
		}

		//@TODO lambda�����ٵ���������� ��ôֱ�Ӱ�������Ա���� ����ForEach?
		void SetOffsetScale(int i, Float value)
		{
			if (value < _offsets[i])
			{
				_offsets[i] = value;
			}
			if (value > _scales[i])
			{
				_scales[i] = value;
			}
		}

		virtual void Finalize()
		{
			for (size_t i = 0; i < _featureNum; i++)
			{
				if (_counts[i] != _total)
				{ //���������prepare������instance�� ����0ֵ, TLCû������� Ӧ����bug ���ֹ�һ��û�е�[0,1]����
			/*		if (0 < _offsets[i] || 0 > _scales[i])
					{
						Pval5(i, _offsets[i], _scales[i], _counts[i], _total);
					}*/
					SetOffsetScale(i, 0);
				}
				_scales[i] -= _offsets[i];
			}
			CheckOffsetAndScale();
		}
	protected:
	private:
		bool _isFirst = true;
		vector<uint64> _counts;
		uint64 _total = 1;
	};

}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__MIN_MAX_NORMALIZER_H_
