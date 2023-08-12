#include "../include/GuillotineBinPack3d.h"
#include "../include/MaxRectsBinPack.h"
#include <iostream>


void testGuillotineBinPack(){
    int bin_width = 1500;
    int bin_height = 1500;
    int bin_depth = 800;

    std::vector<int> box_height_vec{290,290,290,290,290,290,290,290,290,290,290,290,
    230,230,230,230,230,230,230,230,230,230};
    std::vector<int> box_width_vec{510,510,510,510,510,510,510,510,510,510,510,510,
    480,480,480,480,480,480,480,480,480,480};
    std::vector<int> box_depth_vec{210,210,210,210,210,210,210,210,210,210,210,210,
    190,190,190,190,190,190,190,190,190,190};
    
    using rbp::GuillotineBinPack3d;
        
    GuillotineBinPack3d gbp(bin_width, bin_height, bin_depth);
    auto choice_method = GuillotineBinPack3d::RectWorstLongSideFit;
    auto split_method = GuillotineBinPack3d::SplitShorterLeftoverAxis;

    for (int i = 0; i < box_height_vec.size(); i++){
        int box_height = box_height_vec[i];
        int box_width = box_width_vec[i];
        int box_depth = box_depth_vec[i];
        auto rect = gbp.Insert(box_width, box_height, box_depth, true, choice_method, split_method);
        std::cout << "x:" << rect.x << "\ty:" << rect.y << "\tz:" << rect.z << "\twidth:" << rect.width<< "\theight:" << rect.height << "\tdepth:" << rect.depth << std::endl;        
    }
  
}



void testMaxRectsBinPack(){
    int bin_width = 1500;
    int bin_height = 1500;
    int bin_depth = 800;

    std::vector<int> box_height_vec{290,290,290,290,290,290,290,290,290,290,290,290,
    230,230,230,230,230,230,230,230,230,230};
    std::vector<int> box_width_vec{510,510,510,510,510,510,510,510,510,510,510,510,
    480,480,480,480,480,480,480,480,480,480};
    std::vector<int> box_depth_vec{210,210,210,210,210,210,210,210,210,210,210,210,
    190,190,190,190,190,190,190,190,190,190};
    
    using rbp::MaxRectsBinPack;
        
    MaxRectsBinPack mbp(bin_width, bin_height, bin_depth);
    auto choice_method = MaxRectsBinPack::RectBottomLeftRule;    

     
    for (int i = 0; i < box_height_vec.size(); i++){
        int box_height = box_height_vec[i];
        int box_width = box_width_vec[i];
        int box_depth = box_depth_vec[i];
        auto rect = mbp.Insert(box_width, box_height, box_depth, choice_method);

        std::cout << "x:" << rect.x << "\ty:" << rect.y << "\tz:" << rect.z << "\twidth:" << rect.width<< "\theight:" << rect.height << "\tdepth:" << rect.depth << std::endl;        
    }
}

int main(int argc, char* argv[]){
    //testMaxRectsBinPack();
    testGuillotineBinPack();
    return 0;    
}