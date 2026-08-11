#pragma once

// Identifies an internal func_car wheel whose startpos stores a non-uniform
// visual scale. This is intentionally separate from generic studio scaling.
constexpr int FUNC_CAR_WHEEL_MARKER = 0x43415257; // 'CARW'
constexpr int FUNC_CAR_RIGHT_WHEEL_MARKER = 0x43525748; // 'CRWH'
constexpr int FUNC_CAR_BODY_MARKER = 0x43415242; // 'CARB'
constexpr int FUNC_CAR_VIEW_MARKER = 0x43415256; // 'CARV'
