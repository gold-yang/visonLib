#pragma once

#include "ycvtool_global.h"
#include <baseaitool.h>
#include <basetoolcreate.h>
#include "RegisterParam.h"
#include"vision_ALL.h"

//struct defect
//{
//	'背景',
//	'电极',
//	'本体',
//	'本体脏污',
//	'本体白点',
//	'本体崩缺',
//	'本体压痕',
//	'本体沾锡',
//	'电极黑点',
//	'电极内延',
//	'电极缺角',
//	'电极黄点',
//	'电极刮伤',
//	'漏电极',
//	'异物',
//};



class YCVTOOL_EXPORT YCVTool :public BaseAITool
{
	Q_OBJECT
public:
	REGISTERPROPERTY(int, nThreshType, 1) //0代表找黑色,1代表找白色
	REGISTERPROPERTY(int, nProductMode, 0) //0代表电极提取,1代表本体提取,2代表AI提取
	REGISTERPROPERTY(int, nAIDilationX, 3)
	REGISTERPROPERTY(int, nAIDilationY, 3)
	
	//产品
	REGISTERPROPERTY(int, proWidthMax, 1000)
	REGISTERPROPERTY(int, proWidthMin, 20)
	REGISTERPROPERTY(int, proHeightMax, 1000)
	REGISTERPROPERTY(int, proHeightMin, 20)
	REGISTERPROPERTY(int, productExtractThre, 200)
	REGISTERPROPERTY(int, filterArea, 50)
	//电极
	REGISTERPROPERTY(int, polarWidthMax, 1000)
	REGISTERPROPERTY(int, polarWidthMin, 20)
	REGISTERPROPERTY(int, polarHeightMax, 1000)
	REGISTERPROPERTY(int, polarHeightMin, 20)

	//电极脏污
	REGISTERPROPERTY(bool, bUsePolarBlack, true)
	REGISTERPROPERTY(int, PolarBlackThresh, 100)
	REGISTERPROPERTY(int, PolarBlackFilter, 20)
	REGISTERPROPERTY(int, PolarBlackMaxL, 20)
	REGISTERPROPERTY(int, PolarBlackMaxR, 20)
	REGISTERPROPERTY(int, PolarBlackSumL, 20)
	REGISTERPROPERTY(int, PolarBlackSumR, 20)
	REGISTERPROPERTY(int, PolarBlackMaxJudge, 50)
	REGISTERPROPERTY(int, PolarBlackSumJudge, 50)
	//电极内延
	REGISTERPROPERTY(bool, bUsePolarInnerOut, true)
	REGISTERPROPERTY(int, PolarInnerOutThresh, 100)
	REGISTERPROPERTY(int, PolarInnerOutFilter, 20)
	REGISTERPROPERTY(int, PolarInnerOutMax, 20)
	REGISTERPROPERTY(int, PolarInnerOutMaxJudge, 20)
	REGISTERPROPERTY(int, PolarInnerOutSum, 21)
	REGISTERPROPERTY(int, PolarInnerOutSumJudge, 25)
	//电极缺角
	REGISTERPROPERTY(bool, bUsePolarFault, true)
	REGISTERPROPERTY(int, PolarFaultThresh, 100)
	REGISTERPROPERTY(int, PolarFaultFilter, 20)
	REGISTERPROPERTY(int, PolarFaultMax, 20)
	REGISTERPROPERTY(int, PolarFaultMaxJudge, 20)
	REGISTERPROPERTY(int, PolarFaultSum, 21)
	REGISTERPROPERTY(int, PolarFaultSumJudge, 25)
	//电极黄点
	REGISTERPROPERTY(bool, bUsePolarColor, true)
	REGISTERPROPERTY(int, PolarColorThresh, 100)
	REGISTERPROPERTY(int, PolarColorFilter, 20)
	REGISTERPROPERTY(int, PolarColorMax, 20)
	REGISTERPROPERTY(int, PolarColorMaxJudge, 20)
	REGISTERPROPERTY(int, PolarColorSum, 21)
	REGISTERPROPERTY(int, PolarColorSumJudge, 25)
	//电极刮伤
	REGISTERPROPERTY(bool, bUsePolarScratch, true)
	REGISTERPROPERTY(int, PolarScratchThresh, 100)
	REGISTERPROPERTY(int, PolarScratchFilter, 20)
	REGISTERPROPERTY(int, PolarScratchMax, 20)
	REGISTERPROPERTY(int, PolarScratchMaxJudge, 20)
	REGISTERPROPERTY(int, PolarScratchSum, 21)
	REGISTERPROPERTY(int, PolarScratchSumJudge, 25)
	//漏电极
	REGISTERPROPERTY(bool, bUsePolarMiss, true)
	REGISTERPROPERTY(int, PolarMissThresh, 100)
	REGISTERPROPERTY(int, PolarMissFilter, 20)
	REGISTERPROPERTY(int, PolarMissMax, 20)
	REGISTERPROPERTY(int, PolarMissMaxJudge, 20)
	REGISTERPROPERTY(int, PolarMissSum, 21)
	REGISTERPROPERTY(int, PolarMissSumJudge, 25)
	//异物
	REGISTERPROPERTY(bool, bUsePolarMatter, true)
	REGISTERPROPERTY(int, PolarMatterThresh, 100)
	REGISTERPROPERTY(int, PolarMatterFilter, 20)
	REGISTERPROPERTY(int, PolarMatterMax, 20)
	REGISTERPROPERTY(int, PolarMatterMaxJudge, 20)
	REGISTERPROPERTY(int, PolarMatterSum, 21)
	REGISTERPROPERTY(int, PolarMatterSumJudge, 25)


	//本体
	REGISTERPROPERTY(int, bodyWidthMax, 1000)
	REGISTERPROPERTY(int, bodyWidthMin, 20)
	REGISTERPROPERTY(int, bodyHeightMax, 1000)
	REGISTERPROPERTY(int, bodyHeightMin, 20)
	//本体脏污
	REGISTERPROPERTY(bool, bUseBodyBlack, true)
	REGISTERPROPERTY(int, BodyBlackThresh, 100)
	REGISTERPROPERTY(int, BodyBlackFilter, 20)
	REGISTERPROPERTY(int, BodyBlackMax, 20)
	REGISTERPROPERTY(int, BodyBlackSum, 20)
	REGISTERPROPERTY(int, BodyBlackMaxJudge, 21)
	REGISTERPROPERTY(int, BodyBlackSumJudge, 25)
	//本体白点
	REGISTERPROPERTY(bool, bUseBodyWhite, true)
	REGISTERPROPERTY(int, BodyWhiteThresh, 100)
	REGISTERPROPERTY(int, BodyWhiteFilter, 20)
	REGISTERPROPERTY(int, BodyWhiteMax, 20)
	REGISTERPROPERTY(int, BodyWhiteMaxJudge, 21)
	REGISTERPROPERTY(int, BodyWhiteSum, 20)
	REGISTERPROPERTY(int, BodyWhiteSumJudge, 25)
	//本体崩缺
	REGISTERPROPERTY(bool, bUseBodyMiss, true)
	REGISTERPROPERTY(int, BodyMissThresh, 100)
	REGISTERPROPERTY(int, BodyMissFilter, 20)
	REGISTERPROPERTY(int, BodyMissMax, 20)
	REGISTERPROPERTY(int, BodyMissMaxJudge, 21)
	REGISTERPROPERTY(int, BodyMissSum, 20)
	REGISTERPROPERTY(int, BodyMissSumJudge, 25)
	//本体压痕
	REGISTERPROPERTY(bool, bUseBodyIndentation, true)
	REGISTERPROPERTY(int, BodyIndentationThresh, 100)
	REGISTERPROPERTY(int, BodyIndentationFilter, 20)
	REGISTERPROPERTY(int, BodyIndentationMax, 20)
	REGISTERPROPERTY(int, BodyIndentationMaxJudge, 21)
	REGISTERPROPERTY(int, BodyIndentationSum, 20)
	REGISTERPROPERTY(int, BodyIndentationSumJudge, 25)
	//本体沾锡
	REGISTERPROPERTY(bool, bUseBodyBubble, true)
	REGISTERPROPERTY(int, BodyBubbleThresh, 100)
	REGISTERPROPERTY(int, BodyBubbleFilter, 20)
	REGISTERPROPERTY(int, BodyBubbleMax, 20)
	REGISTERPROPERTY(int, BodyBubbleMaxJudge, 21)
	REGISTERPROPERTY(int, BodyBubbleSum, 20)
	REGISTERPROPERTY(int, BodyBubbleSumJudge, 25)


	
	//测量
	REGISTERPROPERTY(int, ProductHeight, 0)
	REGISTERPROPERTY(int, ProductWidth, 0)
	REGISTERPROPERTY(int, PolarHeightL, 0)
	REGISTERPROPERTY(int, PolarHeightR, 0)
	REGISTERPROPERTY(int, PolarWidthL, 0)
	REGISTERPROPERTY(int, PolarWidthR, 0)
	REGISTERPROPERTY(int, BodyHeight, 0)
	REGISTERPROPERTY(int, BodyWidth, 0)



	YCVTool();
	// 通过 BaseTool 继承
	bool processing(ImageCache& origin, ApplicationContext* context) override;
	

	
		
	//AI 函数
	bool productLocation(cv::Mat& gray_image, ApplicationContext* context);
	bool productAIExtractDetection(ApplicationContext* context);
	bool polarAIExtractDetection(cv::Mat& gray_image,ApplicationContext* context);
	bool bodyAIExtractDetection(ApplicationContext* context);
	bool detectAIDetection(cv::Mat& gray_image, ApplicationContext* context);

public:

	cv::Mat mask;
	cv::Mat MatrixR;
	cv::Mat MatrixRInverse;
	cv::Mat MatrixRBody;
	cv::Mat MatrixRInverseBody;

	cv::Rect pre_pro_rect1_;//预先处理产品正矩形
	cv_Rectangle2D pre_pro_rect2_;//预先处理产品旋转矩形

	cv_Rectangle2D body_rect2_;

	cv_Contour2D polar_left_contour2d_;
	cv_Contour2D polar_right_contour2d_;
	cv::Mat pre_pro_region_;//传统处理产品区域
	
	//std::vector<LabelObject> labelRegion;
	std::unordered_map<int, RegionRLE>  labelRegion;

	std::vector<cv::Point>polarLeft;
	std::vector<cv::Point>polarRight;

	std::vector<cv::Point> body;
	cv::Point maskPt;
	cv::Mat mat_mask_pro_;

	//int ProductHeight, ProductWidth, PolarHeightL, PolarHeightR, PolarWidthL, PolarWidthR, BodyHeight, BodyWidth;

	//DEFINENAME(int, ProductHeight, 0)

	//DEFINENAME(int, ProductWidth, 0)
	//DEFINENAME(int, PolarHeightL, 0)
	//DEFINENAME(int, PolarHeightR, 0)
	//DEFINENAME(int, PolarWidthL, 0)
	//DEFINENAME(int, PolarWidthR, 0)
	//DEFINENAME(int, BodyHeight, 0)
	//DEFINENAME(int, BodyWidth, 0)



};
