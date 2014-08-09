/** 
 *  ==============================================================================
 * 
 *          \file   plsa.h
 *
 *        \author   chenghuige  
 *          
 *          \date   2011-07-09 16:46:42.946283
 *  
 *  \Description:   ʵ��һ��ʵ�����ʵĵ����ڴ�汾��PLSA
 *
 *                  Ԥ������  hadoop����
 *                  ����һϵ���ĵ� �д�
 *                  doc_id word_num  word1 weight word2 weight ....
 *                  doc_id Ψһ���� ������˳����
 *
 *
 *       p(z_k|w,d) ����һ��  word_num����doc��word_num����, �ռ�ÿ�ֲ�ͬdoc֮��ɸ���
 *
 *       P(z|d) M���� ֮�͵�ǰ��doc��E�����й�
 *
 *       ����p(z|d)�ĳ�ʼ�������Բ��������ʼ���ķ���������Ϊ��ǰ�ĵ�d�ڸ�������z�ϵĸ���������ġ�
Ϊ�˽����һ���ԣ����ù̶�ֵ��ʼ���ķ�ʽ��Ŀǰ���õ�����p(z)����ʼ��p(z|d)���������ķ�ʽ�£���һ��ʱ
p(z��d,w_i )=p(z|w_i)
��һ�ε���������p(z|d)Ϊ�ı������дʻ�p(z|w)�ľ�ֵ

 ���p(w|z)һ��ʼƽ���Ļ������������TOPICһ������

 ��ʼ���ı��ֱ�Ӧ���ṩ��һЩ��Ϣ��

 p(w|z) = p(z|w)p(w)/ p(z) = p(w) ?

 p(z,d,w) = p(z|d,w) * p(d) * p(w|d)
          = p(d) p(z|d) p(w|d,z)
 p(z|d,w) = p(z|d)p(w|d,z) / p(w|d)
 

 *  ==============================================================================
 */

#ifndef PLSA_H_
#define PLSA_H_
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "common_help.h"
//#include "matrix_help.h"
#include "matrix.h"
#include "debug_help.h"
#include "inverted_index_printer.h"
#include <stdlib.h>
namespace plsa {
using std::string;
using std::vector;
typedef float DP;
//typedef double DP;
typedef vector<std::pair<int, DP> > ContentVec;
typedef std2::Matrix< DP> Mat_;
struct DocInfo
{
    DP nword; //֧�ַ�����
    ContentVec content;
};
class Model
{
public:
    Model(int z_num, int word_num)
    : _znum(z_num), _nword(word_num)
    {

    }
    bool init_doc(const string& file, int max_num)
    {
        ifstream ifs(file.c_str());
        if (!ifs.is_open())
        {
            LOG(ERROR) << "Can not find " << file << endl;
            return false;
        }
        string line;
        static int doc_id = 0;
        while (getline(ifs, line))
        {
            vector<string> vec;
            boost::trim(line);
            boost::algorithm::split(vec, line, boost::is_any_of("\t"));
            _doc_vec.push_back(DocInfo());
            DocInfo& doc_info = _doc_vec[doc_id];
            doc_info.content.resize(TO_INT(vec[1]));
            DP nword = 0;
            for (int i = 2; i < vec.size(); i += 2)
            {
                int index = (i - 2) / 2;
                doc_info.content[index].first = TO_INT(vec[i]);
                doc_info.content[index].second = boost::lexical_cast<DP > (vec[i + 1]);
                nword += doc_info.content[index].second;
            }
            doc_info.nword = nword;
            doc_id++;
            if (doc_id >= max_num)
                return false;
        }
        return true;
    }
    void init_doc_dir(const string& dir, int max_num = std::numeric_limits<int>::max())
    {
        namespace bf = boost::filesystem;
        bf::path input_path(dir);
        bf::directory_iterator iter_e;
        for (bf::directory_iterator iter(input_path); iter != iter_e; ++iter)
        {
            string file_name = (*iter).path().string();
            if (!init_doc(file_name, max_num))
                break;
        }
    }
    void init()
    {
        //        srand(time(NULL));
        //        srand48(time(NULL));
        //init
        _ndoc = _doc_vec.size();
        //        _pdz.setConstant(_ndoc, _znum, 1.0 / _znum);
        //        _pzw.setConstant(_znum, _nword, 1.0 / _nword);
        _pdz.setConstant(_ndoc, _znum, 0);
        _pzw.setConstant(_znum, _nword, 0);
        for (int i = 0; i < _ndoc; i++)
        {
            DP sum = 0.0;
            for (int j = 0; j < _znum; j++)
            {
                _pdz(i, j) = drand48() * drand48();
                sum += _pdz(i, j);
            }
            for (int j = 0; j < _znum; j++)
            {
                _pdz(i, j) /= sum;
            }
        }
        for (int i = 0; i < _znum; i++)
        {
            DP sum = 0.0;
            for (int j = 0; j < _nword; j++)
            {
                _pzw(i, j) = drand48() * drand48();
                sum += _pzw(i, j);
            }
            for (int j = 0; j < _nword; j++)
            {
                _pzw(i, j) /= sum;
            }
        }
        _pzw2.setConstant(_znum, _nword, 0);
        _pz_vec.resize(_znum, 0);
        _pwz.setConstant(_nword, _znum, 0);
    }
    void run(int step = 1)
    {
        init();
        for (int i = 0; i < step; i++) //step
        {
            Pval(i);
            _pzw2.setConstant(0); //TODO
            std::fill(_pz_vec.begin(), _pz_vec.end(), 0);
            for (int did = 0; did < _ndoc; did++) //doc
            {
                //                Pval(did);
                DP nword = _doc_vec[did].nword;
                if (nword < 1) //���0��word��ô��doc������
                    continue;
                ContentVec& vec = _doc_vec[did].content;
                int len = vec.size();
                //--------------------Estep for one doc
                //p(z_k|w,d)
                for (int j = 0; j < len; j++) //word
                {
                    int wid = vec[j].first;
                    //p(w,d)
                    DP pw_d = 0;
                    for (int zid = 0; zid < _znum; zid++)
                    {
                        pw_d += _pdz(did, zid) * _pzw(zid, wid);
                    }
                    for (int zid = 0; zid < _znum; zid++) //z topic
                    {
                        if (pw_d)
                            _pwz(wid, zid) = (_pdz(did, zid) * _pzw(zid, wid)) / pw_d;
                    }
                }
                //-------------------Mstep for one doc
                //---p(z|d) and p(w|z) update,this will be updated after all doc processed in one step
                for (int zid = 0; zid < _znum; zid++)
                {
                    DP sum = 0;
                    for (int j = 0; j < len; j++) //word
                    {
                        int wid = vec[j].first;
                        DP weight = vec[j].second;
                        DP score = weight * _pwz(wid, zid);
                        sum += score;
                        _pzw2(zid, wid) += score;
                    }
                    Pval(sum);
                    _pz_vec[zid] += sum;
                    _pdz(did, zid) = sum / nword; //p(z|d) update
                }
            }
            //after every doc is processed, now finish p(w|z)
            for (int zid = 0; zid < _znum; zid++)
            {
                DP down = _pz_vec[zid];
                for (int wid = 0; wid < _nword; wid++) //word
                {
                    if (down)
                        _pzw(zid, wid) = _pzw2(zid, wid) / down;
                }
            }
        }
    }
    /**
     * չʾĳ��topic id�����top 20��֧�Ŵ�
     */
    template<typename _Dict>
    void print_topic(int zid, const _Dict& dict, int max_num = 200)
    {
        vector<std::pair<int, DP> > vec;
        for (int i = 0; i < _nword; i++)
        {
            vec.push_back(std::make_pair(i, _pzw(zid, i)));
        }
        std::partial_sort(vec.begin(), vec.begin() + max_num, vec.end(), std2::CmpNodeByValue());
        inverted::InvertedIndexPrinter printer(dict);
        printer.print_post_full(vec.begin(), vec.begin() + max_num);
    }

protected:
    int _znum; ///������Ŀ
    vector<DocInfo> _doc_vec; ///����doc����Ϣ
    int _ndoc; ///doc ��Ŀ
    int _nword; ///�ʵ�����Ŀ
    Mat_ _pdz; ///p(d|z)
    Mat_ _pzw; ///p(z|w)
    //temp
    Mat_ _pzw2; ///��MPI�汾�������Ҫ���ݵ�
    vector<DP> _pz_vec; ///���ȴ�СΪ_znum
    Mat_ _pwz; ///Estep p(z|d,w) ÿ��doc����Ҫ Ϊ�˽�ʡ�ռ����doc���� ����doc�䲻�ܲ���
};
}

#endif  //----end of PLSA_H_
