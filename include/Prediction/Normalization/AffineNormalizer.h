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
					//��ҪAffineInit������shiftIndices����scale <=0 ��offset!=0�ĵ�
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

		virtual void Load(string infile) override
		{
			svec lines = read_lines(infile);
			CHECK_GT(lines.size(), 0) << infile;
			int idx = 0;
			CHECK_EQ(parse_string_param("NormalizerType=", lines[idx++]), Name());
			_trunct = parse_bool_param("Trunct=", lines[idx++]);
			_numFeatures = parse_int_param("FeatureNum=", lines[idx++]);
			_offsets.resize(_numFeatures);
			_scales.resize(_numFeatures);
			for (int i = 0; i < _numFeatures; i++)
			{
				string offset, scale;
				split(lines[i + idx], '\t', offset, scale);
				_offsets[i] = DOUBLE(offset);
				_scales[i] = DOUBLE(scale);
			}
			AffineInit();
		}

		virtual void Save(string outfile) override
		{
			ofstream ofs(outfile);
			ofs << "NormalizerType=" << Name() << endl;
			ofs << "Trunct=" << (int)_trunct << endl;
			ofs << "FeatureNum=" << _numFeatures << endl;
			for (int i = 0; i < _numFeatures; i++)
			{
				ofs << _offsets[i] << "\t" << _scales[i] << endl;
			}
		}

		virtual void Begin() override
		{
			_offsets.resize(_numFeatures, 0);
			_scales.resize(_numFeatures, 0);
		}

		virtual void Finish() override
		{

		}

		void AffineInit()
		{
			for (int i = 0; i < _numFeatures; i++)
			{
				if (_scales[i] <= 0)
				{ //����TLC �����ʼ��ֵһ�� ��Ȼά��ԭ�� ��scale ����Ϊ0 @TODO
					//��Ч���� ʼ����6�ı��� ����6 �����0 @TODO ��Ҫ��Ϊ0��
					if (!_featureNames.empty())
					{
						VLOG(4) << "Feature " << i << " : " << _featureNames[i]
							<< " always take value " << _offsets[i];
					}
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
			}

			PVEC(_offsets);
			PVEC(_scales);
			PVEC(_morphIndices);
			PVEC(_shiftIndices);
		}

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<Normalizer>(*this);
			ar & _offsets;
			ar & _scales;
		}

	protected:
		Fvec _offsets;
		Fvec _scales;
		vector<uint64> _counts;
	};

}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__AFFINE_NORMALIZER_H_
