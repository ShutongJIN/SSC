#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>
#include <iterator>
#include "ssc.h"
void zfill(std::string &in_str, size_t len)
{
    while (in_str.size() < len)
    {
        in_str = "0" + in_str;
    }
}
bool cmp(std::tuple<float,int>& score1,std::tuple<float,int>& score2){
    return std::get<0>(score1)>std::get<0>(score2);
}
int main(int argc, char **argv)
{
    std::string conf_file = "../config/config_kitti.yaml";
    if (argc > 1)
    {
        conf_file = argv[1];
    }
    auto data_cfg = YAML::LoadFile(conf_file);
    SSC ssc(conf_file);
    auto file_name_length=data_cfg["file_name_length"].as<int>();
    auto pose_file = data_cfg["eval_top1"]["pose_file"].as<std::string>();
    auto cloud_path = data_cfg["eval_top1"]["cloud_path"].as<std::string>();
    auto label_path = data_cfg["eval_top1"]["label_path"].as<std::string>();
    auto out_file = data_cfg["eval_top1"]["out_file"].as<std::string>();
    std::ifstream f_pose(pose_file);
    std::ofstream f_out(out_file);
    std::istream_iterator<float> start(f_pose), end;
    std::vector<float> pose_temp(start, end);
    std::vector<std::tuple<float, float>> poses;
    for (size_t i = 0; i < pose_temp.size(); i += 12)
    {
        poses.emplace_back(std::make_tuple(pose_temp[i + 3], pose_temp[i + 11]));
    }
    for (size_t i = 51; i < poses.size(); ++i)
    {
        std::cout<<i<<"/"<<poses.size()<<std::endl;
        auto posei = poses[i];
        std::vector<int> match_id;
        for (size_t j = 0; j < i - 50; ++j)
        {
            auto posej = poses[j];
            auto dis = std::sqrt((std::get<1>(posei) - std::get<1>(posej)) * (std::get<1>(posei) - std::get<1>(posej)) + (std::get<0>(posei) - std::get<0>(posej)) * (std::get<0>(posei) - std::get<0>(posej)));
            if (dis <= 5)
            {
                match_id.emplace_back(j);
            }
        }
        if (!match_id.empty())
        {
            std::vector<std::tuple<float, int> > scores;
            for (size_t j = 0; j < i - 50; ++j)
            {
                std::cout<<i<<"/"<<poses.size()<<"\t"<<j<<"/"<<i-50<<std::endl;
                std::string sequ1 = std::to_string(i), sequ2 = std::to_string(j);
                zfill(sequ1, file_name_length);
                zfill(sequ2, file_name_length);
                std::string cloud_file1, cloud_file2, sem_file1, sem_file2;
                cloud_file1 = cloud_path + sequ1;
                cloud_file1 = cloud_file1 + ".bin";
                cloud_file2 = cloud_path + sequ2;
                cloud_file2 = cloud_file2 + ".bin";
                sem_file1 = label_path + sequ1;
                sem_file1 = sem_file1 + ".label";
                sem_file2 = label_path + sequ2;
                sem_file2 = sem_file2 + ".label";
                // std::cout<<cloud_file1<<" "<<sem_file1<<std::endl;
                // std::cout<<cloud_file2<<" "<<sem_file2<<std::endl;
                double angle = 0;
                float diff_x = 0, diff_y = 0;
                auto score = ssc.getScore(cloud_file1, cloud_file2, sem_file1, sem_file2, angle, diff_x, diff_y);
                scores.emplace_back(std::make_tuple(score,j));
            }
            std::sort(scores.begin(),scores.end(),cmp);
            int num=std::min(25,(int)scores.size());
            f_out<<i;
            for(int j=0;j<num;++j){
                auto p=scores[j];
                f_out<<" "<<std::get<1>(p);
            }
            f_out<<std::endl;
        }
    }
    return 0;
}