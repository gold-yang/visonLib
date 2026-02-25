#pragma once

//├── include /
//│   └── halcon /
//│       ├── ALL.h               # 总入口：对外 include 只要这一个
// 
//│       ├── version.h
//│       │
//│       ├── core / # 最底层：不依赖外部库
//│       │   ├── types.h            # Run / Rect / Point / Span / Size / Scalar
//│       │   ├── status.h           # Status / Result<T>
//│       │   ├── error.h            # 异常
//│       │   ├── tuple.h            # HTuple - like
//│       │   ├── config.h           # 编译期开关、平台宏
//│       │   └── utils /
//│       │       ├── clip.h
//│       │       ├── hash.h
//│       │       ├── math.h
//│       │       └── timer.h
//│       │
//│       ├── iconic / # 数据对象（只描述数据，不塞算法）
//│       │   ├── image.h
//│       │   ├── region.h
//│       │   └── xld.h
//│       │
//│       ├── operators / # 公共算子 API（稳定）
//│       │   ├── image /
//│       │   │   ├── threshold.h
//│       │   │   ├── filter.h
//│       │   │   └── morphology.h
//│       │   ├── region /
//│       │   │   ├── build.h        # rle_build 合并为 build（更像 HALCON）
//│       │   │   ├── set.h          # set_ops->set（union / intersect / diff）
//│       │   │   ├── connect.h      # connection->connect（更短更统一）
//│       │   │   ├── select.h       # select_shape->select
//│       │   │   ├── domain.h       # reduce_domain->domain
//│       │   │   └── measure.h      # area / center / bbox 等统计接口
//│       │   ├── xld /
//│       │   │   ├── gen.h          # gen_contour->gen（region->xld）
//│       │   │   ├── ops.h          # contour_ops->ops
//│       │   │   └── geometry.h
//│       │
//│       ├── interop / # 与外部库互操作（隔离依赖）
//│       │   ├── opencv /
//│       │   │   ├── mat_image.h    # cv::Mat < ->Image
//│       │   │   ├── mat_region.h   # cv::Mat < ->Region
//│       │   │   └── draw.h         # drawRegion / drawXld
//│       │
//│       └── process / # 业务过程
//│           ├── xxx.h
//│           ├── detect.h
//│           └── inspect.h


//#include "types.h"
#include "region_core.h"
#include "image_transform.h"
#include "connect.h"
#include "image_transform.h"
#include "xld.h"
#include "rectangle.h"
//#include "region_geometry.h"
//#include "region_render.h"