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
			//////������ ���������ȷ ����������ܴ�������ͬTLC ���� -13 0 �����ٶ�ԭ�� ѡ������ 
			////������ֻ�б��� -13 0  0  0 ���涼�� 0 ���߶��� -5 -4 -3 ���� ��ô���ֵû��ȡ��ʵ�ʵ� 0
			//// -13 -12 0
			////�����Ժ�С�ı������в������ ������ʱ����ҲӰ���С ������������ʱ������[0-n)�ķ�Χ
			vec.ForEach([this](int i, Float value)
			{
				_counts[i]++;
				SetOffsetScale(i, value);
			});
			_total++;
		}
	}

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
			{ //���������prepare������instance�� ����0ֵ
				SetOffsetScale(i, 0);
				if (0 < _offsets[i] || 0 > _scales[i])
				{
					Pval3(i, _offsets[i], _scales[i]);
				}
			}
			_scales[i] -= _offsets[i];
		}
		CheckOffsetScale();
	}
protected:
private:
	bool _isFirst = true;
	vector<uint64> _counts;
	uint64 _total = 1;
};

}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__MIN_MAX_NORMALIZER_H_
