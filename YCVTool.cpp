#include "YCVTool.h"

TOOLCREATE(YCVTool)

YCVTool::YCVTool()
{


}


bool YCVTool::productLocation(cv::Mat& gray_image, ApplicationContext* context) {
	//cv::Mat gray_image = origin;

	//if (origin.empty() || 1 != origin.channels())
	//{
	//	return false;
	//}

	//int productExtractThre = 200;

	cv::threshold(gray_image, pre_pro_region_, productExtractThre, 255, cv::THRESH_BINARY);

	//int filterArea = 50;
	auto shape_contours2D = SelectShapeArea(pre_pro_region_, filterArea, 9999999);
	std::sort(shape_contours2D.begin(), shape_contours2D.end(), [&](const cv_Contour2D& a, const cv_Contour2D& b) {
		return a.area > b.area;
	});

	if (shape_contours2D.size() >= 2) {
		// 区间构造：从begin()开始，到begin()+2结束（左闭右开，刚好取前2个）
		shape_contours2D = std::vector<cv_Contour2D>(shape_contours2D.begin(), shape_contours2D.begin() + 2);
		std::sort(shape_contours2D.begin(), shape_contours2D.end(), [&](const cv_Contour2D& a, const cv_Contour2D& b) {
			return a.boundRect.x > b.boundRect.x;
			});
		polar_left_contour2d_ = shape_contours2D[0];
		polar_right_contour2d_ = shape_contours2D[1];
		polarLeft = polar_left_contour2d_.contour;
		polarRight = polar_right_contour2d_.contour;
	}
	std::vector<cv::Point> polarContour;
	polarContour.insert(polarContour.end(), polarLeft.begin(), polarLeft.end());
	polarContour.insert(polarContour.end(), polarRight.begin(), polarRight.end());
	if (process_name_ == "ProductSeg") {
		for (const auto& contour2D : shape_contours2D) {
			addDrawRegion(pre_pro_region_(contour2D.boundRect), TO_RGB(160, 255, 0), cv::Mat(), rect_det_.tl() + contour2D.boundRect.tl());
		}
	}

	if (shape_contours2D.size() < 2) {
		//没找到电极，无法完成定位
		addNgDetail(tr("产品查找失败"), "productExtractThre", 0);
		return false;
	}

	pre_pro_rect2_ = cv_SmallestRectangle2(polarContour);
	pre_pro_rect2_.offset(rect_det_.tl());
	pre_pro_rect1_ = cv::boundingRect(polarContour) + rect_det_.tl();

	return true;
}

bool YCVTool::productAIExtractDetection(ApplicationContext* context) {

	if (mask.empty() || 1 != mask.channels())
	{
		return false;
	}
	cv::Mat thresh_product;
	cv::threshold(mask, thresh_product, 0, 255, cv::THRESH_BINARY);
	/*std::vector<std::vector<cv::Point>> contours;
	findContours(thresh_product, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);*/
	//改为查找最大轮廓,AI检出存在将背景提取出来的情况
	int maxA;

	cv::Rect rect;
	std::vector<cv::Point> maxCon = findsMaxArea(thresh_product, maxA, rect);
	//未找到产品和定位不到的情况


	pre_pro_rect2_ = cv_SmallestRectangle2(maxCon);

	MatrixR = pre_pro_rect2_.affineMat();
	pre_pro_rect2_.offset(maskPt);
	MatrixRInverse = pre_pro_rect2_.invertAffineMat(); //逆矩阵
	std::vector<cv::Point> rotated;
	cv::transform(maxCon, rotated, MatrixR);
	cv::Rect bbox = cv::boundingRect(rotated);

	ProductHeight = bbox.height;
	ProductWidth = bbox.width;

	addDrawRect(bbox, TO_RGB(160, 0, 255), 2, MatrixRInverse, maskPt, 1);
	//产品长宽检测
	if (process_name_ == "ProductSize") {
		addDrawRect(bbox, TO_RGB(160, 0, 255), 2, MatrixRInverse, maskPt, 0);
	}


	ngJudgeMinMaxValue(BMULX(ProductWidth), proWidthMin, proWidthMax, tr("产品宽异常,检测值:"), "proWidthMin", "proWidthMax", 1, 1);
	ngJudgeMinMaxValue(BMULY(ProductHeight), proHeightMin, proHeightMax, tr("产品高异常,检测值:"), "proHeightMin", "proHeightMax", 1, 2);


	//扫描算法
	//参数
	int nProductNumX = 3; //分割数量 ,=1的时候是产品本身的高,小于2
	int nProductNumY = 2; //分割数量 ,=1的时候是产品本身的高,小于2
	int nPoductInX = 5;   //两端缩进
	int nPoductInY = 5;   //两端缩进

	//扫描,找出最大最小的点，求差值
	std::vector<cv::Point> maxPtsX;
	std::vector<cv::Point> minPtsX;
	std::vector<int> rows, cols;
	for (int i = 0; i < nProductNumX; i++)
	{
		int col = bbox.x + nPoductInX + i * (bbox.width - 2 * nPoductInX) / (nProductNumX - 1);

		int maxV = 0;
		int minV = INT_MAX;
		cv::Point maxPt(0, 0);
		cv::Point minPt(0, 0);
		for (int j = 0; j < rotated.size(); ++j)
		{
			cv::Point c = rotated[j];

			if (c.x == col)
			{
				if (maxV < c.y) {
					maxV = c.y;
					maxPt = c;
				}
				if (minV > c.y) {
					minV = c.y;
					minPt = c;
				}
			}
		}

		maxPtsX.push_back(maxPt);
		minPtsX.push_back(minPt);

	}


	std::vector<cv::Point> maxPtsY;
	std::vector<cv::Point> minPtsY;
	for (int i = 0; i < nProductNumY; i++)
	{
		int row = bbox.y + nPoductInY + i * (bbox.height - 2 * nPoductInY) / (nProductNumY - 1);

		int maxV = 0;
		int minV = INT_MAX;
		cv::Point maxPt(0, 0);
		cv::Point minPt(0, 0);
		for (int j = 0; j < rotated.size(); ++j)
		{
			cv::Point c = rotated[j];

			if (c.y == row)
			{
				if (maxV < c.x) {
					maxV = c.x;
					maxPt = c;
				}
				if (minV > c.x) {
					minV = c.x;
					minPt = c;
				}

			}
		}

		maxPtsY.push_back(maxPt);
		minPtsY.push_back(minPt);

	}






	//cv::Mat drawing = cv::Mat::zeros(mask.size(),CV_8UC1);
	//cv::drawContours(drawing,AllPtss,-1,cv::Scalar(255),-1);


	return true;
}

bool YCVTool::polarAIExtractDetection(cv::Mat& gray_image, ApplicationContext* context) {

	//区分左右电极
	for (auto l : labelRegion) {
		if (l.first == 1) {
			std::vector<std::vector<cv::Point>> contours = GenContourXld(l.second);
			if (contours.size() != 2) {
				//电极数量异常

				return false;
			}

			cv::Rect tempR1 = cv::boundingRect(contours[0]);
			cv::Rect tempR2 = cv::boundingRect(contours[1]);
			if (tempR1.x < tempR2.x) {
				polarLeft = contours[0];
				polarRight = contours[1];
			}
			else {
				polarLeft = contours[1];
				polarRight = contours[0];
			}

		}
	}

	std::vector<cv::Point>rotatedL, rotatedR;
	addDrawContour({ polarLeft,polarRight }, TO_RGB(160, 255, 0), 2, cv::Mat(), maskPt, 1);
	cv::transform(polarLeft, rotatedL, MatrixR);
	cv::transform(polarRight, rotatedR, MatrixR);

	cv::Rect bboxL = cv::boundingRect(rotatedL);
	cv::Rect bboxR = cv::boundingRect(rotatedR);

	std::vector< std::vector<cv::Point>> polarContours;

	polarContours.push_back(polarLeft);
	polarContours.push_back(polarRight);
	if (process_name_ == "PolarSize") {
		addDrawRect(bboxL, TO_RGB(160, 255, 0), 2, MatrixRInverse, maskPt, 0);
		addDrawRect(bboxR, TO_RGB(160, 255, 0), 2, MatrixRInverse, maskPt, 0);
	}


	//电极长宽检测
	PolarWidthL = bboxL.width;
	PolarHeightL = bboxL.height;
	PolarWidthR = bboxR.width;
	PolarHeightR = bboxR.height;

	ngJudgeMinMaxValue(BMULX(PolarWidthL), polarWidthMin, polarWidthMax, tr("左电极宽异常,检测值:"), "polarWidthMin", "polarWidthMax", 1, 1);
	ngJudgeMinMaxValue(BMULX(PolarWidthR), polarWidthMin, polarWidthMax, tr("右电极宽异常,检测值:"), "polarWidthMin", "polarWidthMax", 1, 1);

	ngJudgeMinMaxValue(BMULY(PolarHeightL), polarHeightMin, polarHeightMax, tr("左电极高异常,检测值:"), "polarHeightMin", "polarHeightMax", 1, 2);
	ngJudgeMinMaxValue(BMULY(PolarHeightR), polarHeightMin, polarHeightMax, tr("右电极高异常,检测值:"), "polarHeightMin", "polarHeightMax", 1, 2);


	//for (auto l : labelRegion) {
	//	
	//	//电极脏污
	//	if (l.label == 1 && bUsePolarBlack==1) {
	//		if (process_name_ == "PolarBlack") {
	//			std::vector<cvRegion> res = Connection8(l.region);
	//			for (auto r : res) {
	//			ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - 46, maskPt.y - 46, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, 0, PolarBlackThresh);
	//				if (regT.area > PolarBlackFilter) {
	//					addDrawRegion(regT.bin, TO_RGB(cColorAI1.blue(), cColorAI1.green(), cColorAI1.red()), cv::Mat(), maskPt, 0);
	//				}
	//			}
	//			//int area=l.region.area;
	//			//cv::Rect tempR1 = cv::boundingRect(l.contours[0]);
	//			////cv::Rect tempR2 = cv::boundingRect(l.contours[1]);
	//			//if (tempR1.x < tempR2.x) {
	//			//	polarLeft = l.contours[0];
	//			//	polarRight = l.contours[1];
	//			//}
	//			//else {
	//			//	polarLeft = l.contours[1];
	//			//	polarRight = l.contours[0];
	//			//}
	//		}
	//	}
	//}



	return true;
}

bool YCVTool::bodyAIExtractDetection(ApplicationContext* context) {


	for (auto l : labelRegion) {
		if (l.first == 2) {
			std::vector<std::vector<cv::Point>> contours= GenContourXld(l.second);
			if (contours.size() == 0) {
				//本体数量异常
				return false;
			}
			else if (contours.size() == 1) {

				body = contours[0];
			}
			else {
				std::sort(contours.begin(), contours.end(), [&](const std::vector<cv::Point>& p1, const std::vector<cv::Point>& p2) {
					return cv::contourArea(p1) > cv::contourArea(p2);
					});
				body = contours[0];
			}

			addDrawContour(contours, TO_RGB(255, 0, 255), 2, cv::Mat(), maskPt, 1);

			cv::Rect bbox;
			std::vector<cv::Point>rotated;
			cv::transform(body, rotated, MatrixR);
			bbox = cv::boundingRect(rotated)-cv::Point(1,1);
			BodyWidth = bbox.width;
			BodyHeight = bbox.height;



			//body_rect2_ = cv_SmallestRectangle2(body);
			//body_rect2_.offset(rect_det_.tl());

			//MatrixRBody = body_rect2_.affineMat();
			//body_rect2_.offset(maskPt);
			//MatrixRInverseBody = body_rect2_.invertAffineMat(); //逆矩阵

			


			//body_rect2_.offset(rect_det_.tl());
			//body_rect1_ = cv::boundingRect(body) + rect_det_.tl();

			//cv::getRotationMatrix2D(cv::Point(this->column, this->row), -this->phi * 180 / CV_PI, 1.0);


			if (process_name_ == "BodySize") {
				addDrawRect(bbox, TO_RGB(255, 0, 255), 2, MatrixRInverse, maskPt, 0);
			}
			ngJudgeMinMaxValue(BMULX(BodyWidth), bodyWidthMin, bodyWidthMax, tr("本体宽异常,检测值:"), "bodyWidthMin", "bodyWidthMax", 1, 1);
			ngJudgeMinMaxValue(BMULY(BodyHeight), bodyHeightMin, bodyHeightMax, tr("本体高异常,检测值:"), "bodyHeightMin", "bodyHeightMin", 1, 2);

			break;

		}
	}

	return true;
}

bool YCVTool::detectAIDetection(cv::Mat& gray_image,ApplicationContext* context) {

	for (auto l : labelRegion) {
		//本体脏污
		if (l.first == 3 && bUseBodyBlack == 1) {
			
			std::vector<RegionRLE> res = Connection8(l.second);
			
			int64_t MaxArea = 0;
			int64_t SumArea = 0;
			
			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, 0, PolarBlackThresh);

				if (regT.area > PolarBlackFilter) {
					if (process_name_ == "BodyBlack") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI3.blue(), cColorAI3.green(), cColorAI3.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}

			BodyBlackMax = MaxArea;
			BodyBlackSum = SumArea;

			ngJudgeMaxValue(BodyBlackMax, BodyBlackMaxJudge, tr("本体脏污最大面积,检测值:"), "BodyBlackMax", 1, 1);
			ngJudgeMaxValue(BodyBlackSum, BodyBlackSumJudge, tr("本体脏污总面积,检测值:"), "BodyBlackSum", 1, 1);

		}

		//本体白点
		if (l.first == 4 && bUseBodyWhite == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, BodyWhiteThresh, 255);

				if (regT.area > BodyWhiteFilter) {
					if (process_name_ == "BodyWhite") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI4.blue(), cColorAI4.green(), cColorAI4.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			BodyWhiteMax = MaxArea;
			BodyWhiteSum = SumArea;

			ngJudgeMaxValue(BodyWhiteMax, BodyWhiteMaxJudge, tr("本体白点最大面积,检测值:"), "BodyWhiteMax", 1, 1);
			ngJudgeMaxValue(BodyWhiteSum, BodyWhiteSumJudge, tr("本体白点总面积,检测值:"), "BodyWhiteSum", 1, 1);
		}
		//本体崩缺
		if (l.first == 5 && bUseBodyMiss == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, 0, BodyMissThresh);

				if (regT.area > BodyMissFilter) {
					if (process_name_ == "BodyMiss") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI5.blue(), cColorAI5.green(), cColorAI5.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			BodyMissMax = MaxArea;
			BodyMissSum = SumArea;

			ngJudgeMaxValue(BodyMissMax, BodyMissMaxJudge, tr("本体崩缺最大面积,检测值:"), "BodyMissMax", 1, 1);
			ngJudgeMaxValue(BodyMissSum, BodyMissSumJudge, tr("本体崩缺总面积,检测值:"), "BodyMissSum", 1, 1);
		}
		//本体压痕
		if (l.first == 6 && bUseBodyIndentation == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, BodyIndentationThresh, 255);

				if (regT.area > BodyMissFilter) {
					if (process_name_ == "BodyIndentation") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI6.blue(), cColorAI6.green(), cColorAI6.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			BodyIndentationMax = MaxArea;
			BodyIndentationSum = SumArea;

			ngJudgeMaxValue(BodyIndentationMax, BodyIndentationMaxJudge, tr("本体压痕最大面积,检测值:"), "BodyIndentationMax", 1, 1);
			ngJudgeMaxValue(BodyIndentationSum, BodyIndentationSumJudge, tr("本体压痕总面积,检测值:"), "BodyIndentationSum", 1, 1);
		}
		//本体沾锡
		if (l.first == 7 && bUseBodyBubble == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, BodyBubbleThresh, 255);

				if (regT.area > BodyBubbleFilter) {
					if (process_name_ == "BodyBubble") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI7.blue(), cColorAI7.green(), cColorAI7.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			BodyBubbleMax = MaxArea;
			BodyBubbleSum = SumArea;

			ngJudgeMaxValue(BodyBubbleMax, BodyBubbleMaxJudge, tr("本体沾锡最大面积,检测值:"), "BodyBubbleMax", 1, 1);
			ngJudgeMaxValue(BodyBubbleSum, BodyBubbleSumJudge, tr("本体沾锡总面积,检测值:"), "BodyBubbleSum", 1, 1);
		}

		//电极脏污
		if (l.first == 8 && bUsePolarBlack == 1) {


			//if (process_name_ == "PolarBlack") {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t leftMaxArea = 0;
			int64_t rightMaxArea = 0;
			int64_t leftSumArea = 0;
			int64_t rightSumArea = 0;

			for (auto r : res) {

				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, 0, PolarBlackThresh);

				if (regT.area > PolarBlackFilter) {

					if (process_name_ == "PolarBlack") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI8.blue(), cColorAI8.green(), cColorAI8.red()), cv::Mat(), maskPt, 0);
					}
					//左边
					if (regT.rect.x < pre_pro_rect2_.column) {

						if (leftMaxArea < regT.area) {
							leftMaxArea = regT.area;
						}
						leftSumArea += regT.area;
					}
					else {

						if (rightMaxArea < regT.area) {
							rightMaxArea = regT.area;
						}
						rightSumArea += regT.area;
					}

						

				}
			}
			PolarBlackMaxL = leftMaxArea;
			PolarBlackMaxR = rightMaxArea;
			PolarBlackSumL = leftSumArea;
			PolarBlackSumR = rightSumArea;


			ngJudgeMaxValue(PolarBlackMaxL, PolarBlackMaxJudge, tr("左电极脏污最大面积,检测值:"),"PolarBlackMaxL", 1, 1);
			ngJudgeMaxValue(PolarBlackMaxR, PolarBlackMaxJudge, tr("右电极脏污最大面积,检测值:"), "PolarBlackMaxR", 1, 1);

			ngJudgeMaxValue(PolarBlackSumL, PolarBlackMaxJudge, tr("左电极脏污总面积,检测值:"), "PolarBlackSumL", 1, 1);
			ngJudgeMaxValue(PolarBlackSumR, PolarBlackMaxJudge, tr("右电极脏污总面积,检测值:"), "PolarBlackSumR", 1, 1);

			//ngJudgeMinMaxValue(BMULX(ProductWidth), proWidthMin, proWidthMax, tr("产品宽异常,检测值:"), "proWidthMin", "proWidthMax", 1, 1);
			//}
		}

		//电极内延
		if (l.first == 9 && bUsePolarInnerOut == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, PolarInnerOutThresh, 255);

				if (regT.area > BodyBubbleFilter) {
					if (process_name_ == "PolarInnerOut") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI10.blue(), cColorAI10.green(), cColorAI10.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			PolarInnerOutMax = MaxArea;
			PolarInnerOutSum = SumArea;

			ngJudgeMaxValue(PolarInnerOutMax, PolarInnerOutMaxJudge, tr("电极内延最大面积,检测值:"), "PolarInnerOutMax", 1, 1);
			ngJudgeMaxValue(PolarInnerOutSum, PolarInnerOutSumJudge, tr("电极内延总面积,检测值:"), "PolarInnerOutSum", 1, 1);
		}
		//电极缺角
		if (l.first == 10 && bUsePolarFault == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r,0 , PolarFaultThresh);

				if (regT.area > PolarFaultFilter) {
					if (process_name_ == "PolarFault") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI10.blue(), cColorAI10.green(), cColorAI10.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			PolarFaultMax = MaxArea;
			PolarFaultSum = SumArea;

			ngJudgeMaxValue(PolarFaultMax, PolarInnerOutMaxJudge, tr("电极缺角最大面积,检测值:"), "PolarFaultMax", 1, 1);
			ngJudgeMaxValue(PolarFaultSum, PolarInnerOutSumJudge, tr("电极缺角总面积,检测值:"), "PolarFaultSum", 1, 1);
		}
		//电极黄点
		if (l.first == 11 && bUsePolarColor == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, 0, PolarColorThresh);

				if (regT.area > PolarColorFilter) {
					if (process_name_ == "PolarColor") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI11.blue(), cColorAI11.green(), cColorAI11.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			PolarColorMax = MaxArea;
			PolarColorSum = SumArea;

			ngJudgeMaxValue(PolarColorMax, PolarColorMaxJudge, tr("电极黄点最大面积,检测值:"), "PolarColorMax", 1, 1);
			ngJudgeMaxValue(PolarColorSum, PolarColorSumJudge, tr("电极黄点总面积,检测值:"), "PolarColorSum", 1, 1);
		}
		//电极刮伤
		if (l.first == 12 && bUsePolarScratch == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, 0, PolarScratchThresh);

				if (regT.area > PolarScratchFilter) {
					if (process_name_ == "PolarScratch") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI12.blue(), cColorAI12.green(), cColorAI12.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}

			PolarScratchMax = MaxArea;
			PolarScratchSum = SumArea;

			ngJudgeMaxValue(PolarScratchMax, PolarScratchMaxJudge, tr("电极刮伤最大面积,检测值:"), "PolarScratchMax", 1, 1);
			ngJudgeMaxValue(PolarScratchSum, PolarScratchSumJudge, tr("电极刮伤总面积,检测值:"), "PolarScratchSum", 1, 1);
		}
		//漏电极
		if (l.first == 13 && bUsePolarMiss == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, PolarMissThresh, 255);

				if (regT.area > PolarScratchFilter) {
					if (process_name_ == "PolarMiss") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI13.blue(), cColorAI13.green(), cColorAI13.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			PolarMissMax = MaxArea;
			PolarMissSum = SumArea;

			ngJudgeMaxValue(PolarMissMax, PolarMissMaxJudge, tr("漏电极最大面积,检测值:"), "PolarMissMax", 1, 1);
			ngJudgeMaxValue(PolarMissSum, PolarMissSumJudge, tr("漏电极总面积,检测值:"), "PolarMissSum", 1, 1);
		}
		//异物
		if (l.first == 14 && bUsePolarMatter == 1) {

			std::vector<RegionRLE> res = Connection8(l.second);

			int64_t MaxArea = 0;
			int64_t SumArea = 0;

			for (auto r : res) {
				ThreshResultU8 regT = thresholdInRegionU8(gray_image(cv::Rect(maskPt.x - rect_det_.x, maskPt.y - rect_det_.y, mat_mask_pro_.cols, mat_mask_pro_.rows)), r, PolarMatterThresh, 255);

				if (regT.area > PolarMatterFilter) {
					if (process_name_ == "PolarMatter") {
						addDrawRegion(regT.bin, TO_RGB(cColorAI14.blue(), cColorAI14.green(), cColorAI14.red()), cv::Mat(), maskPt, 0);
					}

					if (MaxArea < regT.area) {
						MaxArea = regT.area;
					}
					SumArea += regT.area;

				}
			}


			PolarMatterMax = MaxArea;
			PolarMatterSum = SumArea;


			ngJudgeMaxValue(PolarMatterMax, PolarMatterMaxJudge, tr("异物最大面积,检测值:"), "PolarMatterMax", 1, 1);
			ngJudgeMaxValue(PolarMatterSum, PolarMatterSumJudge, tr("异物总面积,检测值:"), "PolarMatterSum", 1, 1);

		}


	}
	return true;
}

bool YCVTool::processing(ImageCache& image_cache, ApplicationContext* context)
{
	auto origin = image_cache.cvImage();
	cv::Mat gray_image = origin(rect_det_).clone();

	if (gray_image.channels() == 3) {
		cv::cvtColor(gray_image, gray_image, cv::COLOR_BGR2GRAY);
	}
	if (nProductMode == 2) {
		auto product_rect = rect_det_;
		findProduct(origin, context, product_rect);
		pre_pro_rect2_ = cv_Rectangle2D(ai_rotate_product_.cvRect2());
		pre_pro_rect2_.offset(product_rect.tl());
		pre_pro_rect1_ = cv::boundingRect(pre_pro_rect2_.points(0, 0));
		///绘制AI抓取轮廓
		addDrawContour(pre_pro_rect2_.points(0, 0), TO_RGB(160, 0, 255), 2, cv::Mat(), cv::Point(0, 0), 0, 1);
	}
	else if (!productLocation(gray_image, context)) {
		return true;
	}
	//cv::Mat thresh_mat;
	//cv::threshold(gray_image, thresh_mat, productExtractThre, 255, nThreshType == 0 ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY);
	if (1) {
		addDrawImage(gray_image, cv::Mat(), rect_det_.tl());
	}
	if (bUseAI) {

		//centerP

		//auto pre_pro_rect = pre_pro_rect1_;

		auto pre_pro_rect = pre_pro_rect1_;
		auto t_mask = this->processMMImage(origin, context, pre_pro_rect);
		maskPt = pre_pro_rect.tl();

		if (mat_mask_pro_.size() != t_mask.size()) {
			mat_mask_pro_ = cv::Mat::zeros(t_mask.size(), CV_8U);
		}
		t_mask.convertTo(t_mask, CV_8UC1);
		//扩大一点范围，同时屏蔽AI区域外过检；

		mat_mask_pro_.setTo(255);
		//cv::Mat m1= mat_mask_pro_;
		auto points = pre_pro_rect2_.offset2D(-maskPt).points(nAIDilationX, nAIDilationY);
		fillConvexPoly(mat_mask_pro_, points, cv::Scalar(0));
		t_mask.setTo(0, mat_mask_pro_);
		/*cv::Mat m2 = t_mask;
		cv::Mat m3 = mat_mask_pro_;*/
		//RegionRLE reg;
		//labelRegion = buildAllLabelObjects(t_mask);
		labelRegion = UCharLabelImageToRegions(t_mask);
		//std::vector<cvRegion> vRegion=Connection8(labelRegion[0].region);


		mask = t_mask;
		//if (t_mask.type() == CV_32S) {
		//	t_mask.convertTo(mask, CV_8UC1);
		//}
		/*t_mask.convertTo(mask, CV_8UC1);*/

		productAIExtractDetection(context);

		polarAIExtractDetection(gray_image,context);

		bodyAIExtractDetection(context);

		detectAIDetection(gray_image, context);

	}
	else {
		auto points = pre_pro_rect2_.points(0, 0);
		std::vector<std::vector<cv::Point>> contours = { points };
		addDrawContour(contours, TO_RGB(160, 0, 255), 2, cv::Mat(), cv::Point(0, 0), 1, 1);
		addDrawContour(pre_pro_region_(polar_left_contour2d_.boundRect), TO_RGB(160, 255, 0),2 ,cv::Mat(), rect_det_.tl() + polar_left_contour2d_.boundRect.tl(),1);
		addDrawContour(pre_pro_region_(polar_right_contour2d_.boundRect), TO_RGB(160, 255, 0),2, cv::Mat(), rect_det_.tl() + polar_right_contour2d_.boundRect.tl(),1);
		//contours = { polarLeft,polarRight };
		//addDrawContour(contours, TO_RGB(160, 255, 0), 2, cv::Mat(), rect_det_.tl(), 1, 1);
	}
	BodyHeight = 100;
	return true;
}

