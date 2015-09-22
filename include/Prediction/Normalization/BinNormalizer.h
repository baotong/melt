/**
 *  ==============================================================================
 *
 *          \file   Prediction/Normalization/BinNormalizer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-01 17:08:46.005841
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PREDICTION__NORMALIZATION__BIN_NORMALIZER_H_
#define PREDICTION__NORMALIZATION__BIN_NORMALIZER_H_
#include "common_util.h"
#include "Matrix.h"
#include "Prediction/Normalization/Normalizer.h"
#include "Numeric/BinFinder.h"

namespace gezi {

	//��ȫcopy tlc,��ʵ�ϼ�ʱ0��Ҫ���õ�����binValueҲ���Խ����׸�0 binValue����ֵ���棬���ϡ��Ļ��Ż��ٶ�@TODO
	class BinNormalizer : public Normalizer
	{
	public:
		BinNormalizer()
		{
			ParseArgs();
			_normType = Normalizer::NormType::Bin; //do not BinNormalizer():_normType(NormType::Bin)
			_func = [this](int index, Float& value)
			{
				if (_included[index])
				{
					auto iter = lower_bound(binUpperBounds[index].begin(), binUpperBounds[index].end(), value);
					if (iter != binUpperBounds[index].end())
						value = binValues[index][iter - binUpperBounds[index].begin()];
					else
						THROW("input value is larger than the biggest number in array");
				}
			};
		}

#ifndef MELT_ONLINE
		void ParseArgs();
#else 
		void ParseArgs()
		{

		}
#endif

		BinNormalizer(string path)
		{
			Load(path);
		}

		virtual bool Load(string path) override
		{
			return serialize_util::load(*this, path);
		}

		virtual void Save(string path) override
		{
			serialize_util::save(*this, path);
		}

		virtual string Name() override
		{
			return "BinNormalizer";
		}

		virtual void Begin() override
		{
			Pval(maxBins);
			_included.resize(_numFeatures, true);
			binUpperBounds.resize(_numFeatures);
			values.resize(_numFeatures);
		}

		//TLC�Ĵ���ֱ�ӿ����ڴ� values��С ��ൽ ������Ŀ*������Ŀ ���Բ��ʺ��ı������������Ŀ�޴�� 
		virtual void Process(const Vector& vec) override
		{
			vec.ForEach([this](int idx, Float val) { values[idx].push_back(val); });
			_numProcessedInstances++;
		}

		//@TODO ����Ƚϴ�Ļ� init����ᱬ�ڴ� ����������δ���
		virtual void Finish() override
		{
			init(binValues, _numFeatures, maxBins);
			for (int i = 0; i < _numFeatures; i++)
				for (int j = 0; j < maxBins; j++)
					binValues[i][j] = (Float)j / maxBins;

			uint64 totalSize = _numFeatures * _numProcessedInstances;
			// pre-compute the normalization range for each feature
			ProgressBar pb("BinNormalizer finish", _numFeatures);
			BinFinder binFinder;
#pragma omp parallel for firstprivate(pb) firstprivate(binFinder)
//#pragma omp parallel for firstprivate(binFinder)
			for (int i = 0; i < _numFeatures; i++)
			{
				++pb;
				//values[i].resize(_numProcessedInstances, 0); //�������0
				//binUpperBounds[i] = find_bins(values[i], numBins);
				binUpperBounds[i] = binFinder.FindBins(values[i], _numProcessedInstances, maxBins);
				if (binUpperBounds[i][0] == binUpperBounds[i].back())
				{
					_included[i] = false;
				}
				else if (binUpperBounds[i][0] < 0)
				{
#pragma omp critical					{
						_shiftIndices.push_back(i);
					}
				}
				// reclaculate bin values if too few
				if ((int)binUpperBounds[i].size() < maxBins)
				{
					int numBinsActual = binUpperBounds[i].size();
					binValues[i].resize(numBinsActual);

					for (int j = 0; j < numBinsActual; j++)
						binValues[i][j] = (Float)j / (numBinsActual - 1);
				}
				//if (totalSize > 10000000) //@TODO
				//{
				//	free_memory(values[i]); //�ͷſռ�  @TODO ֱ����һ����ڴ� value[] ����Ŀ�����ȥ �����ͷ� Ч�ʶԱ�?
				//}
			}
#pragma omp parallel
#pragma omp master
			{
				if (omp_get_num_threads() > 1)
					std::sort(_shiftIndices.begin(), _shiftIndices.end());
			}
		}
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
		/*	ar & boost::serialization::base_object<Normalizer>(*this);
			ar & maxBins;
			ar & binUpperBounds;
			ar & binValues;
			ar & _included;*/
			ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Normalizer);
			ar & GEZI_SERIALIZATION_NVP(maxBins);
			ar & GEZI_SERIALIZATION_NVP(binUpperBounds);
			ar & GEZI_SERIALIZATION_NVP(binValues);
			ar & GEZI_SERIALIZATION_NVP(_included);
		}

		const Fmat& BinUpperBounds() const
		{
			return binUpperBounds;
		}

	public:
		//------------args begin
		int maxBins = 1000; //n|Max number of bins per feature
		//------------args end
		// min/max values and ranges for all features
		Fmat binUpperBounds;
	private:
		Fmat binValues;
		Fmat values; //no need to serialize

		BitArray _included;
	};
}  //----end of namespace gezi
CEREAL_REGISTER_TYPE(gezi::BinNormalizer);
#endif  //----end of PREDICTION__NORMALIZATION__BIN_NORMALIZER_H_
