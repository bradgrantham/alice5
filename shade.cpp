#include <iostream>
#include <cstdio>
#include <fstream>
#include <variant>

#include <StandAlone/ResourceLimits.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/intermediate.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.h>
#include "spirv.h"

const int imageWidth = 256;
const int imageHeight = 256;
unsigned char imageBuffer[imageHeight][imageWidth][3];

typedef std::array<float,1> v1float;
typedef std::array<uint32_t,1> v1uint;
typedef std::array<int32_t,1> v1int;
typedef std::array<float,2> v2float;
typedef std::array<uint32_t,2> v2uint;
typedef std::array<int32_t,2> v2int;
typedef std::array<float,3> v3float;
typedef std::array<uint32_t,3> v3uint;
typedef std::array<int32_t,3> v3int;
typedef std::array<float,4> v4float;
typedef std::array<uint32_t,4> v4uint;
typedef std::array<int32_t,4> v4int;

std::map<uint32_t, std::string> OpcodeToString = {
#include "opcode_to_string.h"
};

std::map<glslang::TOperator, std::string> OperatorToString = {
    {glslang::EOpNull, "Null"},
    {glslang::EOpSequence, "Sequence"},
    {glslang::EOpLinkerObjects, "LinkerObjects"},
    {glslang::EOpFunctionCall, "FunctionCall,"},
    {glslang::EOpFunction, "Function"},
    {glslang::EOpParameters, "Parameters"},

    //
    // Unary operators
    //

    {glslang::EOpNegative, "Negative,"},
    {glslang::EOpLogicalNot, "LogicalNot,"},
    {glslang::EOpVectorLogicalNot, "VectorLogicalNot,"},
    {glslang::EOpBitwiseNot, "BitwiseNot,"},

    {glslang::EOpPostIncrement, "PostIncrement,"},
    {glslang::EOpPostDecrement, "PostDecrement,"},
    {glslang::EOpPreIncrement, "PreIncrement,"},
    {glslang::EOpPreDecrement, "PreDecrement,"},

    // (u)int* -> bool
    {glslang::EOpConvInt8ToBool, "ConvInt8ToBool,"},
    {glslang::EOpConvUint8ToBool, "ConvUint8ToBool,"},
    {glslang::EOpConvInt16ToBool, "ConvInt16ToBool,"},
    {glslang::EOpConvUint16ToBool, "ConvUint16ToBool,"},
    {glslang::EOpConvIntToBool, "ConvIntToBool,"},
    {glslang::EOpConvUintToBool, "ConvUintToBool,"},
    {glslang::EOpConvInt64ToBool, "ConvInt64ToBool,"},
    {glslang::EOpConvUint64ToBool, "ConvUint64ToBool,"},

    // float* -> bool
    {glslang::EOpConvFloat16ToBool, "ConvFloat16ToBool,"},
    {glslang::EOpConvFloatToBool, "ConvFloatToBool,"},
    {glslang::EOpConvDoubleToBool, "ConvDoubleToBool,"},

    // bool -> (u)int*
    {glslang::EOpConvBoolToInt8, "ConvBoolToInt8,"},
    {glslang::EOpConvBoolToUint8, "ConvBoolToUint8,"},
    {glslang::EOpConvBoolToInt16, "ConvBoolToInt16,"},
    {glslang::EOpConvBoolToUint16, "ConvBoolToUint16,"},
    {glslang::EOpConvBoolToInt, "ConvBoolToInt,"},
    {glslang::EOpConvBoolToUint, "ConvBoolToUint,"},
    {glslang::EOpConvBoolToInt64, "ConvBoolToInt64,"},
    {glslang::EOpConvBoolToUint64, "ConvBoolToUint64,"},

    // bool -> float*
    {glslang::EOpConvBoolToFloat16, "ConvBoolToFloat16,"},
    {glslang::EOpConvBoolToFloat, "ConvBoolToFloat,"},
    {glslang::EOpConvBoolToDouble, "ConvBoolToDouble,"},

    // int8_t -> (u)int*
    {glslang::EOpConvInt8ToInt16, "ConvInt8ToInt16,"},
    {glslang::EOpConvInt8ToInt, "ConvInt8ToInt,"},
    {glslang::EOpConvInt8ToInt64, "ConvInt8ToInt64,"},
    {glslang::EOpConvInt8ToUint8, "ConvInt8ToUint8,"},
    {glslang::EOpConvInt8ToUint16, "ConvInt8ToUint16,"},
    {glslang::EOpConvInt8ToUint, "ConvInt8ToUint,"},
    {glslang::EOpConvInt8ToUint64, "ConvInt8ToUint64,"},

    // uint8_t -> (u)int*
    {glslang::EOpConvUint8ToInt8, "ConvUint8ToInt8,"},
    {glslang::EOpConvUint8ToInt16, "ConvUint8ToInt16,"},
    {glslang::EOpConvUint8ToInt, "ConvUint8ToInt,"},
    {glslang::EOpConvUint8ToInt64, "ConvUint8ToInt64,"},
    {glslang::EOpConvUint8ToUint16, "ConvUint8ToUint16,"},
    {glslang::EOpConvUint8ToUint, "ConvUint8ToUint,"},
    {glslang::EOpConvUint8ToUint64, "ConvUint8ToUint64,"},

    // int8_t -> float*
    {glslang::EOpConvInt8ToFloat16, "ConvInt8ToFloat16,"},
    {glslang::EOpConvInt8ToFloat, "ConvInt8ToFloat,"},
    {glslang::EOpConvInt8ToDouble, "ConvInt8ToDouble,"},

    // uint8_t -> float*
    {glslang::EOpConvUint8ToFloat16, "ConvUint8ToFloat16,"},
    {glslang::EOpConvUint8ToFloat, "ConvUint8ToFloat,"},
    {glslang::EOpConvUint8ToDouble, "ConvUint8ToDouble,"},

    // int16_t -> (u)int*
    {glslang::EOpConvInt16ToInt8, "ConvInt16ToInt8,"},
    {glslang::EOpConvInt16ToInt, "ConvInt16ToInt,"},
    {glslang::EOpConvInt16ToInt64, "ConvInt16ToInt64,"},
    {glslang::EOpConvInt16ToUint8, "ConvInt16ToUint8,"},
    {glslang::EOpConvInt16ToUint16, "ConvInt16ToUint16,"},
    {glslang::EOpConvInt16ToUint, "ConvInt16ToUint,"},
    {glslang::EOpConvInt16ToUint64, "ConvInt16ToUint64,"},

    // uint16_t -> (u)int*
    {glslang::EOpConvUint16ToInt8, "ConvUint16ToInt8,"},
    {glslang::EOpConvUint16ToInt16, "ConvUint16ToInt16,"},
    {glslang::EOpConvUint16ToInt, "ConvUint16ToInt,"},
    {glslang::EOpConvUint16ToInt64, "ConvUint16ToInt64,"},
    {glslang::EOpConvUint16ToUint8, "ConvUint16ToUint8,"},
    {glslang::EOpConvUint16ToUint, "ConvUint16ToUint,"},
    {glslang::EOpConvUint16ToUint64, "ConvUint16ToUint64,"},

    // int16_t -> float*
    {glslang::EOpConvInt16ToFloat16, "ConvInt16ToFloat16,"},
    {glslang::EOpConvInt16ToFloat, "ConvInt16ToFloat,"},
    {glslang::EOpConvInt16ToDouble, "ConvInt16ToDouble,"},

    // uint16_t -> float*
    {glslang::EOpConvUint16ToFloat16, "ConvUint16ToFloat16,"},
    {glslang::EOpConvUint16ToFloat, "ConvUint16ToFloat,"},
    {glslang::EOpConvUint16ToDouble, "ConvUint16ToDouble,"},

    // int32_t -> (u)int*
    {glslang::EOpConvIntToInt8, "ConvIntToInt8,"},
    {glslang::EOpConvIntToInt16, "ConvIntToInt16,"},
    {glslang::EOpConvIntToInt64, "ConvIntToInt64,"},
    {glslang::EOpConvIntToUint8, "ConvIntToUint8,"},
    {glslang::EOpConvIntToUint16, "ConvIntToUint16,"},
    {glslang::EOpConvIntToUint, "ConvIntToUint,"},
    {glslang::EOpConvIntToUint64, "ConvIntToUint64,"},

    // uint32_t -> (u)int*
    {glslang::EOpConvUintToInt8, "ConvUintToInt8,"},
    {glslang::EOpConvUintToInt16, "ConvUintToInt16,"},
    {glslang::EOpConvUintToInt, "ConvUintToInt,"},
    {glslang::EOpConvUintToInt64, "ConvUintToInt64,"},
    {glslang::EOpConvUintToUint8, "ConvUintToUint8,"},
    {glslang::EOpConvUintToUint16, "ConvUintToUint16,"},
    {glslang::EOpConvUintToUint64, "ConvUintToUint64,"},

    // int32_t -> float*
    {glslang::EOpConvIntToFloat16, "ConvIntToFloat16,"},
    {glslang::EOpConvIntToFloat, "ConvIntToFloat,"},
    {glslang::EOpConvIntToDouble, "ConvIntToDouble,"},

    // uint32_t -> float*
    {glslang::EOpConvUintToFloat16, "ConvUintToFloat16,"},
    {glslang::EOpConvUintToFloat, "ConvUintToFloat,"},
    {glslang::EOpConvUintToDouble, "ConvUintToDouble,"},

    // int64_t -> (u)int*
    {glslang::EOpConvInt64ToInt8, "ConvInt64ToInt8,"},
    {glslang::EOpConvInt64ToInt16, "ConvInt64ToInt16,"},
    {glslang::EOpConvInt64ToInt, "ConvInt64ToInt,"},
    {glslang::EOpConvInt64ToUint8, "ConvInt64ToUint8,"},
    {glslang::EOpConvInt64ToUint16, "ConvInt64ToUint16,"},
    {glslang::EOpConvInt64ToUint, "ConvInt64ToUint,"},
    {glslang::EOpConvInt64ToUint64, "ConvInt64ToUint64,"},

    // uint64_t -> (u)int*
    {glslang::EOpConvUint64ToInt8, "ConvUint64ToInt8,"},
    {glslang::EOpConvUint64ToInt16, "ConvUint64ToInt16,"},
    {glslang::EOpConvUint64ToInt, "ConvUint64ToInt,"},
    {glslang::EOpConvUint64ToInt64, "ConvUint64ToInt64,"},
    {glslang::EOpConvUint64ToUint8, "ConvUint64ToUint8,"},
    {glslang::EOpConvUint64ToUint16, "ConvUint64ToUint16,"},
    {glslang::EOpConvUint64ToUint, "ConvUint64ToUint,"},

    // int64_t -> float*
    {glslang::EOpConvInt64ToFloat16, "ConvInt64ToFloat16,"},
    {glslang::EOpConvInt64ToFloat, "ConvInt64ToFloat,"},
    {glslang::EOpConvInt64ToDouble, "ConvInt64ToDouble,"},

    // uint64_t -> float*
    {glslang::EOpConvUint64ToFloat16, "ConvUint64ToFloat16,"},
    {glslang::EOpConvUint64ToFloat, "ConvUint64ToFloat,"},
    {glslang::EOpConvUint64ToDouble, "ConvUint64ToDouble,"},

    // float16_t -> (u)int*
    {glslang::EOpConvFloat16ToInt8, "ConvFloat16ToInt8,"},
    {glslang::EOpConvFloat16ToInt16, "ConvFloat16ToInt16,"},
    {glslang::EOpConvFloat16ToInt, "ConvFloat16ToInt,"},
    {glslang::EOpConvFloat16ToInt64, "ConvFloat16ToInt64,"},
    {glslang::EOpConvFloat16ToUint8, "ConvFloat16ToUint8,"},
    {glslang::EOpConvFloat16ToUint16, "ConvFloat16ToUint16,"},
    {glslang::EOpConvFloat16ToUint, "ConvFloat16ToUint,"},
    {glslang::EOpConvFloat16ToUint64, "ConvFloat16ToUint64,"},

    // float16_t -> float*
    {glslang::EOpConvFloat16ToFloat, "ConvFloat16ToFloat,"},
    {glslang::EOpConvFloat16ToDouble, "ConvFloat16ToDouble,"},

    // float -> (u)int*
    {glslang::EOpConvFloatToInt8, "ConvFloatToInt8,"},
    {glslang::EOpConvFloatToInt16, "ConvFloatToInt16,"},
    {glslang::EOpConvFloatToInt, "ConvFloatToInt,"},
    {glslang::EOpConvFloatToInt64, "ConvFloatToInt64,"},
    {glslang::EOpConvFloatToUint8, "ConvFloatToUint8,"},
    {glslang::EOpConvFloatToUint16, "ConvFloatToUint16,"},
    {glslang::EOpConvFloatToUint, "ConvFloatToUint,"},
    {glslang::EOpConvFloatToUint64, "ConvFloatToUint64,"},

    // float -> float*
    {glslang::EOpConvFloatToFloat16, "ConvFloatToFloat16,"},
    {glslang::EOpConvFloatToDouble, "ConvFloatToDouble,"},

    // float64 _t-> (u)int*
    {glslang::EOpConvDoubleToInt8, "ConvDoubleToInt8,"},
    {glslang::EOpConvDoubleToInt16, "ConvDoubleToInt16,"},
    {glslang::EOpConvDoubleToInt, "ConvDoubleToInt,"},
    {glslang::EOpConvDoubleToInt64, "ConvDoubleToInt64,"},
    {glslang::EOpConvDoubleToUint8, "ConvDoubleToUint8,"},
    {glslang::EOpConvDoubleToUint16, "ConvDoubleToUint16,"},
    {glslang::EOpConvDoubleToUint, "ConvDoubleToUint,"},
    {glslang::EOpConvDoubleToUint64, "ConvDoubleToUint64,"},

    // float64_t -> float*
    {glslang::EOpConvDoubleToFloat16, "ConvDoubleToFloat16,"},
    {glslang::EOpConvDoubleToFloat, "ConvDoubleToFloat,"},

    // uint64_t <-> pointer
    {glslang::EOpConvUint64ToPtr, "ConvUint64ToPtr,"},
    {glslang::EOpConvPtrToUint64, "ConvPtrToUint64,"},

    //
    // binary operations
    //

    {glslang::EOpAdd, "Add,"},
    {glslang::EOpSub, "Sub,"},
    {glslang::EOpMul, "Mul,"},
    {glslang::EOpDiv, "Div,"},
    {glslang::EOpMod, "Mod,"},
    {glslang::EOpRightShift, "RightShift,"},
    {glslang::EOpLeftShift, "LeftShift,"},
    {glslang::EOpAnd, "And,"},
    {glslang::EOpInclusiveOr, "InclusiveOr,"},
    {glslang::EOpExclusiveOr, "ExclusiveOr,"},
    {glslang::EOpEqual, "Equal,"},
    {glslang::EOpNotEqual, "NotEqual,"},
    {glslang::EOpVectorEqual, "VectorEqual,"},
    {glslang::EOpVectorNotEqual, "VectorNotEqual,"},
    {glslang::EOpLessThan, "LessThan,"},
    {glslang::EOpGreaterThan, "GreaterThan,"},
    {glslang::EOpLessThanEqual, "LessThanEqual,"},
    {glslang::EOpGreaterThanEqual, "GreaterThanEqual,"},
    {glslang::EOpComma, "Comma,"},

    {glslang::EOpVectorTimesScalar, "VectorTimesScalar,"},
    {glslang::EOpVectorTimesMatrix, "VectorTimesMatrix,"},
    {glslang::EOpMatrixTimesVector, "MatrixTimesVector,"},
    {glslang::EOpMatrixTimesScalar, "MatrixTimesScalar,"},

    {glslang::EOpLogicalOr, "LogicalOr,"},
    {glslang::EOpLogicalXor, "LogicalXor,"},
    {glslang::EOpLogicalAnd, "LogicalAnd,"},

    {glslang::EOpIndexDirect, "IndexDirect,"},
    {glslang::EOpIndexIndirect, "IndexIndirect,"},
    {glslang::EOpIndexDirectStruct, "IndexDirectStruct,"},

    {glslang::EOpVectorSwizzle, "VectorSwizzle,"},

    {glslang::EOpMethod, "Method,"},
    {glslang::EOpScoping, "Scoping,"},

    //
    // Built-in functions mapped to operators
    //

    {glslang::EOpRadians, "Radians,"},
    {glslang::EOpDegrees, "Degrees,"},
    {glslang::EOpSin, "Sin,"},
    {glslang::EOpCos, "Cos,"},
    {glslang::EOpTan, "Tan,"},
    {glslang::EOpAsin, "Asin,"},
    {glslang::EOpAcos, "Acos,"},
    {glslang::EOpAtan, "Atan,"},
    {glslang::EOpSinh, "Sinh,"},
    {glslang::EOpCosh, "Cosh,"},
    {glslang::EOpTanh, "Tanh,"},
    {glslang::EOpAsinh, "Asinh,"},
    {glslang::EOpAcosh, "Acosh,"},
    {glslang::EOpAtanh, "Atanh,"},

    {glslang::EOpPow, "Pow,"},
    {glslang::EOpExp, "Exp,"},
    {glslang::EOpLog, "Log,"},
    {glslang::EOpExp2, "Exp2,"},
    {glslang::EOpLog2, "Log2,"},
    {glslang::EOpSqrt, "Sqrt,"},
    {glslang::EOpInverseSqrt, "InverseSqrt,"},

    {glslang::EOpAbs, "Abs,"},
    {glslang::EOpSign, "Sign,"},
    {glslang::EOpFloor, "Floor,"},
    {glslang::EOpTrunc, "Trunc,"},
    {glslang::EOpRound, "Round,"},
    {glslang::EOpRoundEven, "RoundEven,"},
    {glslang::EOpCeil, "Ceil,"},
    {glslang::EOpFract, "Fract,"},
    {glslang::EOpModf, "Modf,"},
    {glslang::EOpMin, "Min,"},
    {glslang::EOpMax, "Max,"},
    {glslang::EOpClamp, "Clamp,"},
    {glslang::EOpMix, "Mix,"},
    {glslang::EOpStep, "Step,"},
    {glslang::EOpSmoothStep, "SmoothStep,"},

    {glslang::EOpIsNan, "IsNan,"},
    {glslang::EOpIsInf, "IsInf,"},

    {glslang::EOpFma, "Fma,"},

    {glslang::EOpFrexp, "Frexp,"},
    {glslang::EOpLdexp, "Ldexp,"},

    {glslang::EOpFloatBitsToInt, "FloatBitsToInt,"},
    {glslang::EOpFloatBitsToUint, "FloatBitsToUint,"},
    {glslang::EOpIntBitsToFloat, "IntBitsToFloat,"},
    {glslang::EOpUintBitsToFloat, "UintBitsToFloat,"},
    {glslang::EOpDoubleBitsToInt64, "DoubleBitsToInt64,"},
    {glslang::EOpDoubleBitsToUint64, "DoubleBitsToUint64,"},
    {glslang::EOpInt64BitsToDouble, "Int64BitsToDouble,"},
    {glslang::EOpUint64BitsToDouble, "Uint64BitsToDouble,"},
    {glslang::EOpFloat16BitsToInt16, "Float16BitsToInt16,"},
    {glslang::EOpFloat16BitsToUint16, "Float16BitsToUint16,"},
    {glslang::EOpInt16BitsToFloat16, "Int16BitsToFloat16,"},
    {glslang::EOpUint16BitsToFloat16, "Uint16BitsToFloat16,"},
    {glslang::EOpPackSnorm2x16, "PackSnorm2x16,"},
    {glslang::EOpUnpackSnorm2x16, "UnpackSnorm2x16,"},
    {glslang::EOpPackUnorm2x16, "PackUnorm2x16,"},
    {glslang::EOpUnpackUnorm2x16, "UnpackUnorm2x16,"},
    {glslang::EOpPackSnorm4x8, "PackSnorm4x8,"},
    {glslang::EOpUnpackSnorm4x8, "UnpackSnorm4x8,"},
    {glslang::EOpPackUnorm4x8, "PackUnorm4x8,"},
    {glslang::EOpUnpackUnorm4x8, "UnpackUnorm4x8,"},
    {glslang::EOpPackHalf2x16, "PackHalf2x16,"},
    {glslang::EOpUnpackHalf2x16, "UnpackHalf2x16,"},
    {glslang::EOpPackDouble2x32, "PackDouble2x32,"},
    {glslang::EOpUnpackDouble2x32, "UnpackDouble2x32,"},
    {glslang::EOpPackInt2x32, "PackInt2x32,"},
    {glslang::EOpUnpackInt2x32, "UnpackInt2x32,"},
    {glslang::EOpPackUint2x32, "PackUint2x32,"},
    {glslang::EOpUnpackUint2x32, "UnpackUint2x32,"},
    {glslang::EOpPackFloat2x16, "PackFloat2x16,"},
    {glslang::EOpUnpackFloat2x16, "UnpackFloat2x16,"},
    {glslang::EOpPackInt2x16, "PackInt2x16,"},
    {glslang::EOpUnpackInt2x16, "UnpackInt2x16,"},
    {glslang::EOpPackUint2x16, "PackUint2x16,"},
    {glslang::EOpUnpackUint2x16, "UnpackUint2x16,"},
    {glslang::EOpPackInt4x16, "PackInt4x16,"},
    {glslang::EOpUnpackInt4x16, "UnpackInt4x16,"},
    {glslang::EOpPackUint4x16, "PackUint4x16,"},
    {glslang::EOpUnpackUint4x16, "UnpackUint4x16,"},
    {glslang::EOpPack16, "Pack16,"},
    {glslang::EOpPack32, "Pack32,"},
    {glslang::EOpPack64, "Pack64,"},
    {glslang::EOpUnpack32, "Unpack32,"},
    {glslang::EOpUnpack16, "Unpack16,"},
    {glslang::EOpUnpack8, "Unpack8,"},

    {glslang::EOpLength, "Length,"},
    {glslang::EOpDistance, "Distance,"},
    {glslang::EOpDot, "Dot,"},
    {glslang::EOpCross, "Cross,"},
    {glslang::EOpNormalize, "Normalize,"},
    {glslang::EOpFaceForward, "FaceForward,"},
    {glslang::EOpReflect, "Reflect,"},
    {glslang::EOpRefract, "Refract,"},

#ifdef AMD_EXTENSIONS
    {glslang::EOpMin3, "Min3,"},
    {glslang::EOpMax3, "Max3,"},
    {glslang::EOpMid3, "Mid3,"},
#endif

    {glslang::EOpDPdx, "DPdx"},
    {glslang::EOpDPdy, "DPdy"},
    {glslang::EOpFwidth, "Fwidth"},
    {glslang::EOpDPdxFine, "DPdxFine"},
    {glslang::EOpDPdyFine, "DPdyFine"},
    {glslang::EOpFwidthFine, "FwidthFine"},
    {glslang::EOpDPdxCoarse, "DPdxCoarse"},
    {glslang::EOpDPdyCoarse, "DPdyCoarse"},
    {glslang::EOpFwidthCoarse, "FwidthCoarse"},

    {glslang::EOpInterpolateAtCentroid, "InterpolateAtCentroid"},
    {glslang::EOpInterpolateAtSample, "InterpolateAtSample"},
    {glslang::EOpInterpolateAtOffset, "InterpolateAtOffset"},

#ifdef AMD_EXTENSIONS
    {glslang::EOpInterpolateAtVertex, "InterpolateAtVertex,"},
#endif

    {glslang::EOpMatrixTimesMatrix, "MatrixTimesMatrix,"},
    {glslang::EOpOuterProduct, "OuterProduct,"},
    {glslang::EOpDeterminant, "Determinant,"},
    {glslang::EOpMatrixInverse, "MatrixInverse,"},
    {glslang::EOpTranspose, "Transpose,"},

    {glslang::EOpFtransform, "Ftransform,"},

    {glslang::EOpNoise, "Noise,"},

    {glslang::EOpEmitVertex, "EmitVertex"},
    {glslang::EOpEndPrimitive, "EndPrimitive"},
    {glslang::EOpEmitStreamVertex, "EmitStreamVertex"},
    {glslang::EOpEndStreamPrimitive, "EndStreamPrimitive"},

    {glslang::EOpBarrier, "Barrier,"},
    {glslang::EOpMemoryBarrier, "MemoryBarrier,"},
    {glslang::EOpMemoryBarrierAtomicCounter, "MemoryBarrierAtomicCounter,"},
    {glslang::EOpMemoryBarrierBuffer, "MemoryBarrierBuffer,"},
    {glslang::EOpMemoryBarrierImage, "MemoryBarrierImage,"},
    {glslang::EOpMemoryBarrierShared, "MemoryBarrierShared"},
    {glslang::EOpGroupMemoryBarrier, "GroupMemoryBarrier"},

    {glslang::EOpBallot, "Ballot,"},
    {glslang::EOpReadInvocation, "ReadInvocation,"},
    {glslang::EOpReadFirstInvocation, "ReadFirstInvocation,"},

    {glslang::EOpAnyInvocation, "AnyInvocation,"},
    {glslang::EOpAllInvocations, "AllInvocations,"},
    {glslang::EOpAllInvocationsEqual, "AllInvocationsEqual,"},

    {glslang::EOpSubgroupGuardStart, "SubgroupGuardStart,"},
    {glslang::EOpSubgroupBarrier, "SubgroupBarrier,"},
    {glslang::EOpSubgroupMemoryBarrier, "SubgroupMemoryBarrier,"},
    {glslang::EOpSubgroupMemoryBarrierBuffer, "SubgroupMemoryBarrierBuffer,"},
    {glslang::EOpSubgroupMemoryBarrierImage, "SubgroupMemoryBarrierImage,"},
    {glslang::EOpSubgroupMemoryBarrierShared, "SubgroupMemoryBarrierShared"},
    {glslang::EOpSubgroupElect, "SubgroupElect,"},
    {glslang::EOpSubgroupAll, "SubgroupAll,"},
    {glslang::EOpSubgroupAny, "SubgroupAny,"},
    {glslang::EOpSubgroupAllEqual, "SubgroupAllEqual,"},
    {glslang::EOpSubgroupBroadcast, "SubgroupBroadcast,"},
    {glslang::EOpSubgroupBroadcastFirst, "SubgroupBroadcastFirst,"},
    {glslang::EOpSubgroupBallot, "SubgroupBallot,"},
    {glslang::EOpSubgroupInverseBallot, "SubgroupInverseBallot,"},
    {glslang::EOpSubgroupBallotBitExtract, "SubgroupBallotBitExtract,"},
    {glslang::EOpSubgroupBallotBitCount, "SubgroupBallotBitCount,"},
    {glslang::EOpSubgroupBallotInclusiveBitCount, "SubgroupBallotInclusiveBitCount,"},
    {glslang::EOpSubgroupBallotExclusiveBitCount, "SubgroupBallotExclusiveBitCount,"},
    {glslang::EOpSubgroupBallotFindLSB, "SubgroupBallotFindLSB,"},
    {glslang::EOpSubgroupBallotFindMSB, "SubgroupBallotFindMSB,"},
    {glslang::EOpSubgroupShuffle, "SubgroupShuffle,"},
    {glslang::EOpSubgroupShuffleXor, "SubgroupShuffleXor,"},
    {glslang::EOpSubgroupShuffleUp, "SubgroupShuffleUp,"},
    {glslang::EOpSubgroupShuffleDown, "SubgroupShuffleDown,"},
    {glslang::EOpSubgroupAdd, "SubgroupAdd,"},
    {glslang::EOpSubgroupMul, "SubgroupMul,"},
    {glslang::EOpSubgroupMin, "SubgroupMin,"},
    {glslang::EOpSubgroupMax, "SubgroupMax,"},
    {glslang::EOpSubgroupAnd, "SubgroupAnd,"},
    {glslang::EOpSubgroupOr, "SubgroupOr,"},
    {glslang::EOpSubgroupXor, "SubgroupXor,"},
    {glslang::EOpSubgroupInclusiveAdd, "SubgroupInclusiveAdd,"},
    {glslang::EOpSubgroupInclusiveMul, "SubgroupInclusiveMul,"},
    {glslang::EOpSubgroupInclusiveMin, "SubgroupInclusiveMin,"},
    {glslang::EOpSubgroupInclusiveMax, "SubgroupInclusiveMax,"},
    {glslang::EOpSubgroupInclusiveAnd, "SubgroupInclusiveAnd,"},
    {glslang::EOpSubgroupInclusiveOr, "SubgroupInclusiveOr,"},
    {glslang::EOpSubgroupInclusiveXor, "SubgroupInclusiveXor,"},
    {glslang::EOpSubgroupExclusiveAdd, "SubgroupExclusiveAdd,"},
    {glslang::EOpSubgroupExclusiveMul, "SubgroupExclusiveMul,"},
    {glslang::EOpSubgroupExclusiveMin, "SubgroupExclusiveMin,"},
    {glslang::EOpSubgroupExclusiveMax, "SubgroupExclusiveMax,"},
    {glslang::EOpSubgroupExclusiveAnd, "SubgroupExclusiveAnd,"},
    {glslang::EOpSubgroupExclusiveOr, "SubgroupExclusiveOr,"},
    {glslang::EOpSubgroupExclusiveXor, "SubgroupExclusiveXor,"},
    {glslang::EOpSubgroupClusteredAdd, "SubgroupClusteredAdd,"},
    {glslang::EOpSubgroupClusteredMul, "SubgroupClusteredMul,"},
    {glslang::EOpSubgroupClusteredMin, "SubgroupClusteredMin,"},
    {glslang::EOpSubgroupClusteredMax, "SubgroupClusteredMax,"},
    {glslang::EOpSubgroupClusteredAnd, "SubgroupClusteredAnd,"},
    {glslang::EOpSubgroupClusteredOr, "SubgroupClusteredOr,"},
    {glslang::EOpSubgroupClusteredXor, "SubgroupClusteredXor,"},
    {glslang::EOpSubgroupQuadBroadcast, "SubgroupQuadBroadcast,"},
    {glslang::EOpSubgroupQuadSwapHorizontal, "SubgroupQuadSwapHorizontal,"},
    {glslang::EOpSubgroupQuadSwapVertical, "SubgroupQuadSwapVertical,"},
    {glslang::EOpSubgroupQuadSwapDiagonal, "SubgroupQuadSwapDiagonal,"},

#ifdef NV_EXTENSIONS
    {glslang::EOpSubgroupPartition, "SubgroupPartition,"},
    {glslang::EOpSubgroupPartitionedAdd, "SubgroupPartitionedAdd,"},
    {glslang::EOpSubgroupPartitionedMul, "SubgroupPartitionedMul,"},
    {glslang::EOpSubgroupPartitionedMin, "SubgroupPartitionedMin,"},
    {glslang::EOpSubgroupPartitionedMax, "SubgroupPartitionedMax,"},
    {glslang::EOpSubgroupPartitionedAnd, "SubgroupPartitionedAnd,"},
    {glslang::EOpSubgroupPartitionedOr, "SubgroupPartitionedOr,"},
    {glslang::EOpSubgroupPartitionedXor, "SubgroupPartitionedXor,"},
    {glslang::EOpSubgroupPartitionedInclusiveAdd, "SubgroupPartitionedInclusiveAdd,"},
    {glslang::EOpSubgroupPartitionedInclusiveMul, "SubgroupPartitionedInclusiveMul,"},
    {glslang::EOpSubgroupPartitionedInclusiveMin, "SubgroupPartitionedInclusiveMin,"},
    {glslang::EOpSubgroupPartitionedInclusiveMax, "SubgroupPartitionedInclusiveMax,"},
    {glslang::EOpSubgroupPartitionedInclusiveAnd, "SubgroupPartitionedInclusiveAnd,"},
    {glslang::EOpSubgroupPartitionedInclusiveOr, "SubgroupPartitionedInclusiveOr,"},
    {glslang::EOpSubgroupPartitionedInclusiveXor, "SubgroupPartitionedInclusiveXor,"},
    {glslang::EOpSubgroupPartitionedExclusiveAdd, "SubgroupPartitionedExclusiveAdd,"},
    {glslang::EOpSubgroupPartitionedExclusiveMul, "SubgroupPartitionedExclusiveMul,"},
    {glslang::EOpSubgroupPartitionedExclusiveMin, "SubgroupPartitionedExclusiveMin,"},
    {glslang::EOpSubgroupPartitionedExclusiveMax, "SubgroupPartitionedExclusiveMax,"},
    {glslang::EOpSubgroupPartitionedExclusiveAnd, "SubgroupPartitionedExclusiveAnd,"},
    {glslang::EOpSubgroupPartitionedExclusiveOr, "SubgroupPartitionedExclusiveOr,"},
    {glslang::EOpSubgroupPartitionedExclusiveXor, "SubgroupPartitionedExclusiveXor,"},
#endif

    {glslang::EOpSubgroupGuardStop, "SubgroupGuardStop,"},

#ifdef AMD_EXTENSIONS
    {glslang::EOpMinInvocations, "MinInvocations,"},
    {glslang::EOpMaxInvocations, "MaxInvocations,"},
    {glslang::EOpAddInvocations, "AddInvocations,"},
    {glslang::EOpMinInvocationsNonUniform, "MinInvocationsNonUniform,"},
    {glslang::EOpMaxInvocationsNonUniform, "MaxInvocationsNonUniform,"},
    {glslang::EOpAddInvocationsNonUniform, "AddInvocationsNonUniform,"},
    {glslang::EOpMinInvocationsInclusiveScan, "MinInvocationsInclusiveScan,"},
    {glslang::EOpMaxInvocationsInclusiveScan, "MaxInvocationsInclusiveScan,"},
    {glslang::EOpAddInvocationsInclusiveScan, "AddInvocationsInclusiveScan,"},
    {glslang::EOpMinInvocationsInclusiveScanNonUniform, "MinInvocationsInclusiveScanNonUniform,"},
    {glslang::EOpMaxInvocationsInclusiveScanNonUniform, "MaxInvocationsInclusiveScanNonUniform,"},
    {glslang::EOpAddInvocationsInclusiveScanNonUniform, "AddInvocationsInclusiveScanNonUniform,"},
    {glslang::EOpMinInvocationsExclusiveScan, "MinInvocationsExclusiveScan,"},
    {glslang::EOpMaxInvocationsExclusiveScan, "MaxInvocationsExclusiveScan,"},
    {glslang::EOpAddInvocationsExclusiveScan, "AddInvocationsExclusiveScan,"},
    {glslang::EOpMinInvocationsExclusiveScanNonUniform, "MinInvocationsExclusiveScanNonUniform,"},
    {glslang::EOpMaxInvocationsExclusiveScanNonUniform, "MaxInvocationsExclusiveScanNonUniform,"},
    {glslang::EOpAddInvocationsExclusiveScanNonUniform, "AddInvocationsExclusiveScanNonUniform,"},
    {glslang::EOpSwizzleInvocations, "SwizzleInvocations,"},
    {glslang::EOpSwizzleInvocationsMasked, "SwizzleInvocationsMasked,"},
    {glslang::EOpWriteInvocation, "WriteInvocation,"},
    {glslang::EOpMbcnt, "Mbcnt,"},

    {glslang::EOpCubeFaceIndex, "CubeFaceIndex,"},
    {glslang::EOpCubeFaceCoord, "CubeFaceCoord,"},
    {glslang::EOpTime, "Time,"},
#endif

    {glslang::EOpAtomicAdd, "AtomicAdd,"},
    {glslang::EOpAtomicMin, "AtomicMin,"},
    {glslang::EOpAtomicMax, "AtomicMax,"},
    {glslang::EOpAtomicAnd, "AtomicAnd,"},
    {glslang::EOpAtomicOr, "AtomicOr,"},
    {glslang::EOpAtomicXor, "AtomicXor,"},
    {glslang::EOpAtomicExchange, "AtomicExchange,"},
    {glslang::EOpAtomicCompSwap, "AtomicCompSwap,"},
    {glslang::EOpAtomicLoad, "AtomicLoad,"},
    {glslang::EOpAtomicStore, "AtomicStore,"},

    {glslang::EOpAtomicCounterIncrement, "AtomicCounterIncrement"},
    {glslang::EOpAtomicCounterDecrement, "AtomicCounterDecrement"},
    {glslang::EOpAtomicCounter, "AtomicCounter,"},
    {glslang::EOpAtomicCounterAdd, "AtomicCounterAdd,"},
    {glslang::EOpAtomicCounterSubtract, "AtomicCounterSubtract,"},
    {glslang::EOpAtomicCounterMin, "AtomicCounterMin,"},
    {glslang::EOpAtomicCounterMax, "AtomicCounterMax,"},
    {glslang::EOpAtomicCounterAnd, "AtomicCounterAnd,"},
    {glslang::EOpAtomicCounterOr, "AtomicCounterOr,"},
    {glslang::EOpAtomicCounterXor, "AtomicCounterXor,"},
    {glslang::EOpAtomicCounterExchange, "AtomicCounterExchange,"},
    {glslang::EOpAtomicCounterCompSwap, "AtomicCounterCompSwap,"},

    {glslang::EOpAny, "Any,"},
    {glslang::EOpAll, "All,"},

    {glslang::EOpCooperativeMatrixLoad, "CooperativeMatrixLoad,"},
    {glslang::EOpCooperativeMatrixStore, "CooperativeMatrixStore,"},
    {glslang::EOpCooperativeMatrixMulAdd, "CooperativeMatrixMulAdd,"},

    //
    // Branch
    //

    {glslang::EOpKill, "Kill"},
    {glslang::EOpReturn, "Return,"},
    {glslang::EOpBreak, "Break,"},
    {glslang::EOpContinue, "Continue,"},
    {glslang::EOpCase, "Case,"},
    {glslang::EOpDefault, "Default,"},

    //
    // Constructors
    //

    {glslang::EOpConstructGuardStart, "ConstructGuardStart,"},
    {glslang::EOpConstructInt, "ConstructInt"},
    {glslang::EOpConstructUint, "ConstructUint,"},
    {glslang::EOpConstructInt8, "ConstructInt8,"},
    {glslang::EOpConstructUint8, "ConstructUint8,"},
    {glslang::EOpConstructInt16, "ConstructInt16,"},
    {glslang::EOpConstructUint16, "ConstructUint16,"},
    {glslang::EOpConstructInt64, "ConstructInt64,"},
    {glslang::EOpConstructUint64, "ConstructUint64,"},
    {glslang::EOpConstructBool, "ConstructBool,"},
    {glslang::EOpConstructFloat, "ConstructFloat,"},
    {glslang::EOpConstructDouble, "ConstructDouble,"},
    {glslang::EOpConstructVec2, "ConstructVec2,"},
    {glslang::EOpConstructVec3, "ConstructVec3,"},
    {glslang::EOpConstructVec4, "ConstructVec4,"},
    {glslang::EOpConstructDVec2, "ConstructDVec2,"},
    {glslang::EOpConstructDVec3, "ConstructDVec3,"},
    {glslang::EOpConstructDVec4, "ConstructDVec4,"},
    {glslang::EOpConstructBVec2, "ConstructBVec2,"},
    {glslang::EOpConstructBVec3, "ConstructBVec3,"},
    {glslang::EOpConstructBVec4, "ConstructBVec4,"},
    {glslang::EOpConstructI8Vec2, "ConstructI8Vec2,"},
    {glslang::EOpConstructI8Vec3, "ConstructI8Vec3,"},
    {glslang::EOpConstructI8Vec4, "ConstructI8Vec4,"},
    {glslang::EOpConstructU8Vec2, "ConstructU8Vec2,"},
    {glslang::EOpConstructU8Vec3, "ConstructU8Vec3,"},
    {glslang::EOpConstructU8Vec4, "ConstructU8Vec4,"},
    {glslang::EOpConstructI16Vec2, "ConstructI16Vec2,"},
    {glslang::EOpConstructI16Vec3, "ConstructI16Vec3,"},
    {glslang::EOpConstructI16Vec4, "ConstructI16Vec4,"},
    {glslang::EOpConstructU16Vec2, "ConstructU16Vec2,"},
    {glslang::EOpConstructU16Vec3, "ConstructU16Vec3,"},
    {glslang::EOpConstructU16Vec4, "ConstructU16Vec4,"},
    {glslang::EOpConstructIVec2, "ConstructIVec2,"},
    {glslang::EOpConstructIVec3, "ConstructIVec3,"},
    {glslang::EOpConstructIVec4, "ConstructIVec4,"},
    {glslang::EOpConstructUVec2, "ConstructUVec2,"},
    {glslang::EOpConstructUVec3, "ConstructUVec3,"},
    {glslang::EOpConstructUVec4, "ConstructUVec4,"},
    {glslang::EOpConstructI64Vec2, "ConstructI64Vec2,"},
    {glslang::EOpConstructI64Vec3, "ConstructI64Vec3,"},
    {glslang::EOpConstructI64Vec4, "ConstructI64Vec4,"},
    {glslang::EOpConstructU64Vec2, "ConstructU64Vec2,"},
    {glslang::EOpConstructU64Vec3, "ConstructU64Vec3,"},
    {glslang::EOpConstructU64Vec4, "ConstructU64Vec4,"},
    {glslang::EOpConstructMat2x2, "ConstructMat2x2,"},
    {glslang::EOpConstructMat2x3, "ConstructMat2x3,"},
    {glslang::EOpConstructMat2x4, "ConstructMat2x4,"},
    {glslang::EOpConstructMat3x2, "ConstructMat3x2,"},
    {glslang::EOpConstructMat3x3, "ConstructMat3x3,"},
    {glslang::EOpConstructMat3x4, "ConstructMat3x4,"},
    {glslang::EOpConstructMat4x2, "ConstructMat4x2,"},
    {glslang::EOpConstructMat4x3, "ConstructMat4x3,"},
    {glslang::EOpConstructMat4x4, "ConstructMat4x4,"},
    {glslang::EOpConstructDMat2x2, "ConstructDMat2x2,"},
    {glslang::EOpConstructDMat2x3, "ConstructDMat2x3,"},
    {glslang::EOpConstructDMat2x4, "ConstructDMat2x4,"},
    {glslang::EOpConstructDMat3x2, "ConstructDMat3x2,"},
    {glslang::EOpConstructDMat3x3, "ConstructDMat3x3,"},
    {glslang::EOpConstructDMat3x4, "ConstructDMat3x4,"},
    {glslang::EOpConstructDMat4x2, "ConstructDMat4x2,"},
    {glslang::EOpConstructDMat4x3, "ConstructDMat4x3,"},
    {glslang::EOpConstructDMat4x4, "ConstructDMat4x4,"},
    {glslang::EOpConstructIMat2x2, "ConstructIMat2x2,"},
    {glslang::EOpConstructIMat2x3, "ConstructIMat2x3,"},
    {glslang::EOpConstructIMat2x4, "ConstructIMat2x4,"},
    {glslang::EOpConstructIMat3x2, "ConstructIMat3x2,"},
    {glslang::EOpConstructIMat3x3, "ConstructIMat3x3,"},
    {glslang::EOpConstructIMat3x4, "ConstructIMat3x4,"},
    {glslang::EOpConstructIMat4x2, "ConstructIMat4x2,"},
    {glslang::EOpConstructIMat4x3, "ConstructIMat4x3,"},
    {glslang::EOpConstructIMat4x4, "ConstructIMat4x4,"},
    {glslang::EOpConstructUMat2x2, "ConstructUMat2x2,"},
    {glslang::EOpConstructUMat2x3, "ConstructUMat2x3,"},
    {glslang::EOpConstructUMat2x4, "ConstructUMat2x4,"},
    {glslang::EOpConstructUMat3x2, "ConstructUMat3x2,"},
    {glslang::EOpConstructUMat3x3, "ConstructUMat3x3,"},
    {glslang::EOpConstructUMat3x4, "ConstructUMat3x4,"},
    {glslang::EOpConstructUMat4x2, "ConstructUMat4x2,"},
    {glslang::EOpConstructUMat4x3, "ConstructUMat4x3,"},
    {glslang::EOpConstructUMat4x4, "ConstructUMat4x4,"},
    {glslang::EOpConstructBMat2x2, "ConstructBMat2x2,"},
    {glslang::EOpConstructBMat2x3, "ConstructBMat2x3,"},
    {glslang::EOpConstructBMat2x4, "ConstructBMat2x4,"},
    {glslang::EOpConstructBMat3x2, "ConstructBMat3x2,"},
    {glslang::EOpConstructBMat3x3, "ConstructBMat3x3,"},
    {glslang::EOpConstructBMat3x4, "ConstructBMat3x4,"},
    {glslang::EOpConstructBMat4x2, "ConstructBMat4x2,"},
    {glslang::EOpConstructBMat4x3, "ConstructBMat4x3,"},
    {glslang::EOpConstructBMat4x4, "ConstructBMat4x4,"},
    {glslang::EOpConstructFloat16, "ConstructFloat16,"},
    {glslang::EOpConstructF16Vec2, "ConstructF16Vec2,"},
    {glslang::EOpConstructF16Vec3, "ConstructF16Vec3,"},
    {glslang::EOpConstructF16Vec4, "ConstructF16Vec4,"},
    {glslang::EOpConstructF16Mat2x2, "ConstructF16Mat2x2,"},
    {glslang::EOpConstructF16Mat2x3, "ConstructF16Mat2x3,"},
    {glslang::EOpConstructF16Mat2x4, "ConstructF16Mat2x4,"},
    {glslang::EOpConstructF16Mat3x2, "ConstructF16Mat3x2,"},
    {glslang::EOpConstructF16Mat3x3, "ConstructF16Mat3x3,"},
    {glslang::EOpConstructF16Mat3x4, "ConstructF16Mat3x4,"},
    {glslang::EOpConstructF16Mat4x2, "ConstructF16Mat4x2,"},
    {glslang::EOpConstructF16Mat4x3, "ConstructF16Mat4x3,"},
    {glslang::EOpConstructF16Mat4x4, "ConstructF16Mat4x4,"},
    {glslang::EOpConstructStruct, "ConstructStruct,"},
    {glslang::EOpConstructTextureSampler, "ConstructTextureSampler,"},
    {glslang::EOpConstructNonuniform, "ConstructNonuniform"},
    {glslang::EOpConstructReference, "ConstructReference,"},
    {glslang::EOpConstructCooperativeMatrix, "ConstructCooperativeMatrix,"},
    {glslang::EOpConstructGuardEnd, "ConstructGuardEnd,"},

    //
    // moves
    //

    {glslang::EOpAssign, "Assign,"},
    {glslang::EOpAddAssign, "AddAssign,"},
    {glslang::EOpSubAssign, "SubAssign,"},
    {glslang::EOpMulAssign, "MulAssign,"},
    {glslang::EOpVectorTimesMatrixAssign, "VectorTimesMatrixAssign,"},
    {glslang::EOpVectorTimesScalarAssign, "VectorTimesScalarAssign,"},
    {glslang::EOpMatrixTimesScalarAssign, "MatrixTimesScalarAssign,"},
    {glslang::EOpMatrixTimesMatrixAssign, "MatrixTimesMatrixAssign,"},
    {glslang::EOpDivAssign, "DivAssign,"},
    {glslang::EOpModAssign, "ModAssign,"},
    {glslang::EOpAndAssign, "AndAssign,"},
    {glslang::EOpInclusiveOrAssign, "InclusiveOrAssign,"},
    {glslang::EOpExclusiveOrAssign, "ExclusiveOrAssign,"},
    {glslang::EOpLeftShiftAssign, "LeftShiftAssign,"},
    {glslang::EOpRightShiftAssign, "RightShiftAssign,"},

    //
    // Array operators
    //

    // Can apply to arrays, vectors, or matrices.
    // Can be decomposed to a constant at compile time, but this does not always happen,
    // due to link-time effects. So, consumer can expect either a link-time sized or
    // run-time sized array.
    {glslang::EOpArrayLength, "ArrayLength,"},

    //
    // Image operations
    //

    {glslang::EOpImageGuardBegin, "ImageGuardBegin,"},

    {glslang::EOpImageQuerySize, "ImageQuerySize,"},
    {glslang::EOpImageQuerySamples, "ImageQuerySamples,"},
    {glslang::EOpImageLoad, "ImageLoad,"},
    {glslang::EOpImageStore, "ImageStore,"},
#ifdef AMD_EXTENSIONS
    {glslang::EOpImageLoadLod, "ImageLoadLod,"},
    {glslang::EOpImageStoreLod, "ImageStoreLod,"},
#endif
    {glslang::EOpImageAtomicAdd, "ImageAtomicAdd,"},
    {glslang::EOpImageAtomicMin, "ImageAtomicMin,"},
    {glslang::EOpImageAtomicMax, "ImageAtomicMax,"},
    {glslang::EOpImageAtomicAnd, "ImageAtomicAnd,"},
    {glslang::EOpImageAtomicOr, "ImageAtomicOr,"},
    {glslang::EOpImageAtomicXor, "ImageAtomicXor,"},
    {glslang::EOpImageAtomicExchange, "ImageAtomicExchange,"},
    {glslang::EOpImageAtomicCompSwap, "ImageAtomicCompSwap,"},
    {glslang::EOpImageAtomicLoad, "ImageAtomicLoad,"},
    {glslang::EOpImageAtomicStore, "ImageAtomicStore,"},

    {glslang::EOpSubpassLoad, "SubpassLoad,"},
    {glslang::EOpSubpassLoadMS, "SubpassLoadMS,"},
    {glslang::EOpSparseImageLoad, "SparseImageLoad,"},
#ifdef AMD_EXTENSIONS
    {glslang::EOpSparseImageLoadLod, "SparseImageLoadLod,"},
#endif

    {glslang::EOpImageGuardEnd, "ImageGuardEnd,"},

    //
    // Texture operations
    //

    {glslang::EOpTextureGuardBegin, "TextureGuardBegin,"},

    {glslang::EOpTextureQuerySize, "TextureQuerySize,"},
    {glslang::EOpTextureQueryLod, "TextureQueryLod,"},
    {glslang::EOpTextureQueryLevels, "TextureQueryLevels,"},
    {glslang::EOpTextureQuerySamples, "TextureQuerySamples,"},

    {glslang::EOpSamplingGuardBegin, "SamplingGuardBegin,"},

    {glslang::EOpTexture, "Texture,"},
    {glslang::EOpTextureProj, "TextureProj,"},
    {glslang::EOpTextureLod, "TextureLod,"},
    {glslang::EOpTextureOffset, "TextureOffset,"},
    {glslang::EOpTextureFetch, "TextureFetch,"},
    {glslang::EOpTextureFetchOffset, "TextureFetchOffset,"},
    {glslang::EOpTextureProjOffset, "TextureProjOffset,"},
    {glslang::EOpTextureLodOffset, "TextureLodOffset,"},
    {glslang::EOpTextureProjLod, "TextureProjLod,"},
    {glslang::EOpTextureProjLodOffset, "TextureProjLodOffset,"},
    {glslang::EOpTextureGrad, "TextureGrad,"},
    {glslang::EOpTextureGradOffset, "TextureGradOffset,"},
    {glslang::EOpTextureProjGrad, "TextureProjGrad,"},
    {glslang::EOpTextureProjGradOffset, "TextureProjGradOffset,"},
    {glslang::EOpTextureGather, "TextureGather,"},
    {glslang::EOpTextureGatherOffset, "TextureGatherOffset,"},
    {glslang::EOpTextureGatherOffsets, "TextureGatherOffsets,"},
    {glslang::EOpTextureClamp, "TextureClamp,"},
    {glslang::EOpTextureOffsetClamp, "TextureOffsetClamp,"},
    {glslang::EOpTextureGradClamp, "TextureGradClamp,"},
    {glslang::EOpTextureGradOffsetClamp, "TextureGradOffsetClamp,"},
#ifdef AMD_EXTENSIONS
    {glslang::EOpTextureGatherLod, "TextureGatherLod,"},
    {glslang::EOpTextureGatherLodOffset, "TextureGatherLodOffset,"},
    {glslang::EOpTextureGatherLodOffsets, "TextureGatherLodOffsets,"},
    {glslang::EOpFragmentMaskFetch, "FragmentMaskFetch,"},
    {glslang::EOpFragmentFetch, "FragmentFetch,"},
#endif

    {glslang::EOpSparseTextureGuardBegin, "SparseTextureGuardBegin,"},

    {glslang::EOpSparseTexture, "SparseTexture,"},
    {glslang::EOpSparseTextureLod, "SparseTextureLod,"},
    {glslang::EOpSparseTextureOffset, "SparseTextureOffset,"},
    {glslang::EOpSparseTextureFetch, "SparseTextureFetch,"},
    {glslang::EOpSparseTextureFetchOffset, "SparseTextureFetchOffset,"},
    {glslang::EOpSparseTextureLodOffset, "SparseTextureLodOffset,"},
    {glslang::EOpSparseTextureGrad, "SparseTextureGrad,"},
    {glslang::EOpSparseTextureGradOffset, "SparseTextureGradOffset,"},
    {glslang::EOpSparseTextureGather, "SparseTextureGather,"},
    {glslang::EOpSparseTextureGatherOffset, "SparseTextureGatherOffset,"},
    {glslang::EOpSparseTextureGatherOffsets, "SparseTextureGatherOffsets,"},
    {glslang::EOpSparseTexelsResident, "SparseTexelsResident,"},
    {glslang::EOpSparseTextureClamp, "SparseTextureClamp,"},
    {glslang::EOpSparseTextureOffsetClamp, "SparseTextureOffsetClamp,"},
    {glslang::EOpSparseTextureGradClamp, "SparseTextureGradClamp,"},
    {glslang::EOpSparseTextureGradOffsetClamp, "SparseTextureGradOffsetClamp,"},
#ifdef AMD_EXTENSIONS
    {glslang::EOpSparseTextureGatherLod, "SparseTextureGatherLod,"},
    {glslang::EOpSparseTextureGatherLodOffset, "SparseTextureGatherLodOffset,"},
    {glslang::EOpSparseTextureGatherLodOffsets, "SparseTextureGatherLodOffsets,"},
#endif

    {glslang::EOpSparseTextureGuardEnd, "SparseTextureGuardEnd,"},

#ifdef NV_EXTENSIONS
    {glslang::EOpImageFootprintGuardBegin, "ImageFootprintGuardBegin,"},
    {glslang::EOpImageSampleFootprintNV, "ImageSampleFootprintNV,"},
    {glslang::EOpImageSampleFootprintClampNV, "ImageSampleFootprintClampNV,"},
    {glslang::EOpImageSampleFootprintLodNV, "ImageSampleFootprintLodNV,"},
    {glslang::EOpImageSampleFootprintGradNV, "ImageSampleFootprintGradNV,"},
    {glslang::EOpImageSampleFootprintGradClampNV, "ImageSampleFootprintGradClampNV,"},
    {glslang::EOpImageFootprintGuardEnd, "ImageFootprintGuardEnd,"},
#endif
    {glslang::EOpSamplingGuardEnd, "SamplingGuardEnd,"},
    {glslang::EOpTextureGuardEnd, "TextureGuardEnd,"},

    //
    // Integer operations
    //

    {glslang::EOpAddCarry, "AddCarry,"},
    {glslang::EOpSubBorrow, "SubBorrow,"},
    {glslang::EOpUMulExtended, "UMulExtended,"},
    {glslang::EOpIMulExtended, "IMulExtended,"},
    {glslang::EOpBitfieldExtract, "BitfieldExtract,"},
    {glslang::EOpBitfieldInsert, "BitfieldInsert,"},
    {glslang::EOpBitFieldReverse, "BitFieldReverse,"},
    {glslang::EOpBitCount, "BitCount,"},
    {glslang::EOpFindLSB, "FindLSB,"},
    {glslang::EOpFindMSB, "FindMSB,"},

#ifdef NV_EXTENSIONS
    {glslang::EOpTraceNV, "TraceNV,"},
    {glslang::EOpReportIntersectionNV, "ReportIntersectionNV,"},
    {glslang::EOpIgnoreIntersectionNV, "IgnoreIntersectionNV,"},
    {glslang::EOpTerminateRayNV, "TerminateRayNV,"},
    {glslang::EOpExecuteCallableNV, "ExecuteCallableNV,"},
    {glslang::EOpWritePackedPrimitiveIndices4x8NV, "WritePackedPrimitiveIndices4x8NV,"},
#endif
    //
    // HLSL operations
    //

    {glslang::EOpClip, "Clip"},
    {glslang::EOpIsFinite, "IsFinite,"},
    {glslang::EOpLog10, "Log10"},
    {glslang::EOpRcp, "Rcp"},
    {glslang::EOpSaturate, "Saturate"},
    {glslang::EOpSinCos, "SinCos"},
    {glslang::EOpGenMul, "GenMul"},
    {glslang::EOpDst, "Dst"},
    {glslang::EOpInterlockedAdd, "InterlockedAdd"},
    {glslang::EOpInterlockedAnd, "InterlockedAnd"},
    {glslang::EOpInterlockedCompareExchange, "InterlockedCompareExchange"},
    {glslang::EOpInterlockedCompareStore, "InterlockedCompareStore"},
    {glslang::EOpInterlockedExchange, "InterlockedExchange"},
    {glslang::EOpInterlockedMax, "InterlockedMax"},
    {glslang::EOpInterlockedMin, "InterlockedMin"},
    {glslang::EOpInterlockedOr, "InterlockedOr"},
    {glslang::EOpInterlockedXor, "InterlockedXor"},
    {glslang::EOpAllMemoryBarrierWithGroupSync, "AllMemoryBarrierWithGroupSync"},
    {glslang::EOpDeviceMemoryBarrier, "DeviceMemoryBarrier"},
    {glslang::EOpDeviceMemoryBarrierWithGroupSync, "DeviceMemoryBarrierWithGroupSync"},
    {glslang::EOpWorkgroupMemoryBarrier, "WorkgroupMemoryBarrier"},
    {glslang::EOpWorkgroupMemoryBarrierWithGroupSync, "WorkgroupMemoryBarrierWithGroupSync"},
    {glslang::EOpEvaluateAttributeSnapped, "EvaluateAttributeSnapped"},
    {glslang::EOpF32tof16, "F32tof16"},
    {glslang::EOpF16tof32, "F16tof32"},
    {glslang::EOpLit, "Lit"},
    {glslang::EOpTextureBias, "TextureBias"},
    {glslang::EOpAsDouble, "AsDouble"},
    {glslang::EOpD3DCOLORtoUBYTE4, "D3DCOLORtoUBYTE4"},

    {glslang::EOpMethodSample, "MethodSample"},
    {glslang::EOpMethodSampleBias, "MethodSampleBias"},
    {glslang::EOpMethodSampleCmp, "MethodSampleCmp"},
    {glslang::EOpMethodSampleCmpLevelZero, "MethodSampleCmpLevelZero"},
    {glslang::EOpMethodSampleGrad, "MethodSampleGrad"},
    {glslang::EOpMethodSampleLevel, "MethodSampleLevel"},
    {glslang::EOpMethodLoad, "MethodLoad"},
    {glslang::EOpMethodGetDimensions, "MethodGetDimensions"},
    {glslang::EOpMethodGetSamplePosition, "MethodGetSamplePosition"},
    {glslang::EOpMethodGather, "MethodGather"},
    {glslang::EOpMethodCalculateLevelOfDetail, "MethodCalculateLevelOfDetail"},
    {glslang::EOpMethodCalculateLevelOfDetailUnclamped, "MethodCalculateLevelOfDetailUnclamped"},

    // Load already defined above for textures
    {glslang::EOpMethodLoad2, "MethodLoad2"},
    {glslang::EOpMethodLoad3, "MethodLoad3"},
    {glslang::EOpMethodLoad4, "MethodLoad4"},
    {glslang::EOpMethodStore, "MethodStore"},
    {glslang::EOpMethodStore2, "MethodStore2"},
    {glslang::EOpMethodStore3, "MethodStore3"},
    {glslang::EOpMethodStore4, "MethodStore4"},
    {glslang::EOpMethodIncrementCounter, "MethodIncrementCounter"},
    {glslang::EOpMethodDecrementCounter, "MethodDecrementCounter"},
    // {glslang::EOpMethodAppend is defined for geo shaders below, "MethodAppend is defined for geo shaders below"},
    {glslang::EOpMethodConsume, "MethodConsume,"},

    // SM5 texture methods
    {glslang::EOpMethodGatherRed, "MethodGatherRed"},
    {glslang::EOpMethodGatherGreen, "MethodGatherGreen"},
    {glslang::EOpMethodGatherBlue, "MethodGatherBlue"},
    {glslang::EOpMethodGatherAlpha, "MethodGatherAlpha"},
    {glslang::EOpMethodGatherCmp, "MethodGatherCmp"},
    {glslang::EOpMethodGatherCmpRed, "MethodGatherCmpRed"},
    {glslang::EOpMethodGatherCmpGreen, "MethodGatherCmpGreen"},
    {glslang::EOpMethodGatherCmpBlue, "MethodGatherCmpBlue"},
    {glslang::EOpMethodGatherCmpAlpha, "MethodGatherCmpAlpha"},

    // geometry methods
    {glslang::EOpMethodAppend, "MethodAppend"},
    {glslang::EOpMethodRestartStrip, "MethodRestartStrip"},

    // matrix
    {glslang::EOpMatrixSwizzle, "MatrixSwizzle"},

    // SM6 wave ops
    {glslang::EOpWaveGetLaneCount, "WaveGetLaneCount"},
    {glslang::EOpWaveGetLaneIndex, "WaveGetLaneIndex"},
    {glslang::EOpWaveActiveCountBits, "WaveActiveCountBits"},
    {glslang::EOpWavePrefixCountBits, "WavePrefixCountBits"},
};

std::map<glslang::TBasicType, std::string> BasicTypeToString = {
    {glslang::EbtVoid, "Void"},
    {glslang::EbtFloat, "Float"},
    {glslang::EbtDouble, "Double"},
    {glslang::EbtFloat16, "Float16"},
    {glslang::EbtInt8, "Int8"},
    {glslang::EbtUint8, "Uint8"},
    {glslang::EbtInt16, "Int16"},
    {glslang::EbtUint16, "Uint16"},
    {glslang::EbtInt, "Int"},
    {glslang::EbtUint, "Uint"},
    {glslang::EbtInt64, "Int64"},
    {glslang::EbtUint64, "Uint64"},
    {glslang::EbtBool, "Bool"},
    {glslang::EbtAtomicUint, "AtomicUint"},
    {glslang::EbtSampler, "Sampler"},
    {glslang::EbtStruct, "Struct"},
    {glslang::EbtBlock, "Block"},

#ifdef NV_EXTENSIONS
    {glslang::EbtAccStructNV, "AccStructNV"},
#endif

    {glslang::EbtReference, "Reference"},

    {glslang::EbtString, "String"},
};

// A variable in memory, either global or within a function's frame.
struct Variable
{
    // Key into the "types" map.
    uint32_t type;

    // One of the values of SpvStorageClass (input, output, uniform, function, etc.).
    uint32_t storageClass;

    // XXX Not sure, or NO_INITIALIZER.
    uint32_t initializer;

    // Offset into the memory for the specific storage class.
    size_t offset;
};


// Entry point for the shader.
struct EntryPoint
{
    // What kind of shader. Must be SpvExecutionModelFragment.
    uint32_t executionModel;

    // Name of the function to call. This is typically "main".
    std::string name;

    // XXX Don't know.
    std::vector<uint32_t> interfaceIds;

    // XXX Don't know what an execution mode is.
    std::map<uint32_t, std::vector<uint32_t>> executionModesToOperands;
};

// XXX A decoration tells us something about a type or variable, but I'm not sure what, maybe
// how to access it from outside the shader and the offset of fields within a struct.
struct Decoration
{
    // The ID of a type or variable.
    uint32_t decoration;

    // XXX Specific operands for this decoration.
    std::vector<uint32_t> operands;
};

// Information about the source code for the shader.
struct Source
{
    // What language the shader was written in.
    uint32_t language;

    // XXX Version of the shading language.
    uint32_t version;

    // XXX Dunno.
    uint32_t file;

    // XXX Filename of the shader source?
    std::string source;
};

// XXX Unused.
struct MemberName
{
    uint32_t type;
    uint32_t member;
    std::string name;
};

// Represents a "void" type.
struct TypeVoid
{
    // XXX Not sure why we need this. Not allowed to have an empty struct?
    int foo;
};

// Represents an "int" type.
struct TypeInt
{
    // Number of bits in the int.
    uint32_t width;

    // Whether signed.
    uint32_t signedness;
};

// Represents a "float" type.
struct TypeFloat
{
    // Number of bits in the float.
    uint32_t width;
};

// Represents a vector type.
struct TypeVector
{
    // Type of the element. This is a key in the "types" map.
    uint32_t type;

    // Number of elements.
    uint32_t count;
};

// Represents a function type.
struct TypeFunction
{
    // Type of the return value. This is a key in the "types" map.
    uint32_t returnType;

    // Type of each parameter, in order from left to right. These are keys in the "types" map.
    std::vector<uint32_t> parameterTypes;
};

// Represents a struct type.
struct TypeStruct
{
    // Type of each element, in order. These are keys in the "types" map.
    std::vector<uint32_t> memberTypes;
};

// Represents a pointer type.
struct TypePointer
{
    // Type of the data being pointed to. This is a key in the "types" map.
    uint32_t type;

    // Storage class of the data. One of the values of SpvStorageClass (input,
    // output, uniform, function, etc.).
    uint32_t storageClass;
};

// Represents an image type.
struct TypeImage
{
    // XXX document these.
    uint32_t sampledType;
    uint32_t dim;
    uint32_t depth;
    uint32_t arrayed;
    uint32_t ms;
    uint32_t sampled;
    uint32_t imageFormat;
    uint32_t accessQualifier;
};

// XXX Dunno.
struct TypeSampledImage
{
    // Type of the image. This is a key in the "types" map. XXX yes?
    uint32_t imageType;
};

// Union of all known types.
typedef std::variant<TypeVoid, TypeFloat, TypePointer, TypeFunction, TypeVector, TypeInt, TypeStruct, TypeImage, TypeSampledImage> Type;

// A constant value hard-coded in the program or implicitly used by the compiler.
struct Constant
{
    // Type of the constant. This is a key in the "types" map.
    uint32_t type;

    // Value of the bits. Floats are presented bitwise.
    uint32_t value;
};

// A function parameter. This is on the receiving side, to set aside an SSA slot
// for the value received from the caller.
struct FunctionParameter
{
    // Type of the parameter. This is a key in the "types" map.
    uint32_t type;

    // Name of the parameter. This is a key in the "names" array.
    uint32_t id;
};

#include "opcode_structs.h"

const uint32_t NO_MEMORY_ACCESS_SEMANTIC = 0xFFFFFFFF;

typedef std::variant<
#include "opcode_variant.h"
> Instruction;

struct Function
{
    uint32_t id;
    uint32_t resultType;
    uint32_t functionControl;
    uint32_t functionType;
    uint32_t start;
};

struct RegisterPointer
{
    uint32_t type;
    uint32_t storageClass;
    size_t offset; // offset into memory, not into Region
};

struct RegisterObject
{
    uint32_t type;
    size_t size;
    unsigned char *data;
    
    RegisterObject(const RegisterObject &other) 
    {
        if(false)std::cout << "move ctor allocated to " << this << " as " << &data << "\n";
        type = other.type;
        size = other.size;
        data = new unsigned char[size];
        std::copy(other.data, other.data + size, data);
    }

    RegisterObject(uint32_t type_, size_t size_) :
        type(type_),
        size(size_),
        data(new unsigned char[size_])
    {
        if(false)std::cout << "ctor allocated to " << this << " as " << &data << "\n";
    }

    RegisterObject() :
        type(0xFFFFFFFF),
        size(0),
        data(nullptr)
    {
        if(false)std::cout << "ctor empty RegisterObject " << this << " \n";
    }

    ~RegisterObject()
    {
        if(false)std::cout << "dtor deleting from " << this << " as " << &data << "\n";
        delete[] data;
    }

    RegisterObject& operator=(const RegisterObject &other)
    {
        if(false)std::cout << "operator=\n";
        if(this != &other) {
            if(false)std::cout << "op= deleting from " << this << " as " << &data << "\n";
            delete[] data;
            type = other.type;
            size = other.size;
            data = new unsigned char[size];
            if(false)std::cout << "op= allocated to " << this << " as " << &data << "\n";
            std::copy(other.data, other.data + size, data);
        }
        return *this;
    }
};

typedef std::variant<RegisterPointer, RegisterObject> Register;

const uint32_t SOURCE_NO_FILE = 0xFFFFFFFF;
const uint32_t NO_INITIALIZER = 0xFFFFFFFF;
const uint32_t NO_ACCESS_QUALIFIER = 0xFFFFFFFF;
const uint32_t EXECUTION_ENDED = 0xFFFFFFFF;
const uint32_t NO_RETURN_REGISTER = 0xFFFFFFFF;

struct MemoryRegion
{
    size_t base;
    size_t size;
    size_t top;
    MemoryRegion() :
        base(0),
        size(0),
        top(0)
    {}
    MemoryRegion(size_t base_, size_t size_) :
        base(base_),
        size(size_),
        top(base_)
    {}
};

// helper type for visitor
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct Interpreter 
{
    bool throwOnUnimplemented;
    bool verbose;
    std::set<uint32_t> capabilities;

    // main id-to-thingie map containing extinstsets, types, variables, etc
    // secondary maps of entryPoint, decorations, names, etc

    std::map<uint32_t, std::string> extInstSets;
    std::map<uint32_t, std::string> strings;
    uint32_t memoryModel;
    uint32_t addressingModel;
    std::map<uint32_t, EntryPoint> entryPoints;
    std::map<uint32_t, Decoration> decorations;
    std::map<uint32_t, std::string> names;
    std::vector<Source> sources;
    std::vector<std::string> processes;
    std::map<uint32_t, std::map<uint32_t, std::string> > memberNames;
    std::map<uint32_t, std::map<uint32_t, Decoration> > memberDecorations;
    std::map<uint32_t, Type> types;
    std::map<uint32_t, size_t> typeSizes; // XXX put into Type
    std::map<uint32_t, Constant> constants;
    std::map<uint32_t, Variable> variables;
    std::map<uint32_t, Function> functions;
    std::vector<Instruction> code;

    Function* currentFunction;
    std::map<uint32_t, uint32_t> labels;
    Function* mainFunction; 

    std::map<uint32_t, MemoryRegion> memoryRegions;
    unsigned char *memory;

    std::map<uint32_t, Register> registers;
    RegisterObject& allocRegisterObject(uint32_t id, uint32_t type)
    {
        RegisterObject r {type, typeSizes[type]};
        registers[id] = r;
        return std::get<RegisterObject>(registers[id]);
    }

    Interpreter(bool throwOnUnimplemented_, bool verbose_) :
        throwOnUnimplemented(throwOnUnimplemented_),
        verbose(verbose_),
        currentFunction(nullptr)
    {
        size_t base = 0;
        auto anotherRegion = [&base](size_t size){MemoryRegion r(base, size); base += size; return r;};
        memoryRegions[SpvStorageClassUniformConstant] = anotherRegion(1024);
        memoryRegions[SpvStorageClassInput] = anotherRegion(1024);
        memoryRegions[SpvStorageClassUniform] = anotherRegion(1024);
        memoryRegions[SpvStorageClassOutput] = anotherRegion(1024);
        memoryRegions[SpvStorageClassWorkgroup] = anotherRegion(0);
        memoryRegions[SpvStorageClassCrossWorkgroup] = anotherRegion(0);
            // I'll put intermediates in "Private" storage:
        memoryRegions[SpvStorageClassPrivate] = anotherRegion(65536);
        memoryRegions[SpvStorageClassFunction] = anotherRegion(16384);
        memoryRegions[SpvStorageClassGeneric] = anotherRegion(0);
        memoryRegions[SpvStorageClassPushConstant] = anotherRegion(0);
        memoryRegions[SpvStorageClassAtomicCounter] = anotherRegion(0);
        memoryRegions[SpvStorageClassImage] = anotherRegion(0);
        memoryRegions[SpvStorageClassStorageBuffer] = anotherRegion(0);
        memory = new unsigned char[base];
    }

    ~Interpreter()
    {
        delete[] memory;
    }

    size_t allocate(SpvStorageClass clss, uint32_t type)
    {
        MemoryRegion& reg = memoryRegions[clss];
        if(false) {
            std::cout << "allocate from " << clss << " type " << type << "\n";
            std::cout << "region at " << reg.base << " size " << reg.size << " top " << reg.top << "\n";
            std::cout << "object is size " << typeSizes[type] << "\n";
        }
        assert(reg.top + typeSizes[type] <= reg.base + reg.size);
        size_t offset = reg.top;
        reg.top += typeSizes[type];
        return offset;
    }
    size_t allocate(uint32_t clss, uint32_t type)
    {
        return allocate(static_cast<SpvStorageClass>(clss), type);
    }

    void copy(uint32_t type, size_t src, size_t dst)
    {
        std::copy(memory + src, memory + src + typeSizes[type], memory + dst);
    }

    template <class T>
    T& objectInClassAt(SpvStorageClass clss, size_t offset)
    {
        return *reinterpret_cast<T*>(memory + memoryRegions[clss].base + offset);
    }

    template <class T>
    T& objectAt(size_t offset)
    {
        return *reinterpret_cast<T*>(memory + offset);
    }

    template <class T>
    T& objectAt(unsigned char* data)
    {
        return *reinterpret_cast<T*>(data);
    }

    template <class T>
    T& registerAs(int id)
    {
        return *reinterpret_cast<T*>(std::get<RegisterObject>(registers[id]).data);
    }

    uint32_t getConstituentType(uint32_t t, int i)
    {
        const Type& type = types[t];
        if(std::holds_alternative<TypeVector>(type)) {
            return std::get<TypeVector>(type).type;
        } else if (std::holds_alternative<TypeStruct>(type)) {
            return std::get<TypeStruct>(type).memberTypes[i];
        // XXX } else if (std::holds_alternative<TypeArray>(type)) {
        } else {
            std::cout << type.index() << "\n";
            assert(false && "getConstituentType of invalid type?!");
        }
        return 0; // not reached
    }

    void dumpTypeAt(const Type& type, unsigned char *ptr)
    {
        std::visit(overloaded {
            [&](const TypeVoid& type) { std::cout << "{}"; },
            [&](const TypeFloat& type) { std::cout << objectAt<float>(ptr); },
            [&](const TypeInt& type) {
                if(type.signedness) {
                    std::cout << objectAt<int32_t>(ptr);
                } else {
                    std::cout << objectAt<uint32_t>(ptr);
                }
            },
            [&](const TypePointer& type) { std::cout << "(ptr)" << objectAt<uint32_t>(ptr); },
            [&](const TypeFunction& type) { std::cout << "function"; },
            [&](const TypeImage& type) { std::cout << "image"; },
            [&](const TypeSampledImage& type) { std::cout << "sampledimage"; },
            [&](const TypeVector& type) {
                std::cout << "<";
                for(int i = 0; i < type.count; i++) {
                    dumpTypeAt(types[type.type], ptr);
                    ptr += typeSizes[type.type];
                    if(i < type.count - 1)
                        std::cout << ", ";
                }
                std::cout << ">";
            },
            [&](const TypeStruct& type) {
                std::cout << "{";
                for(int i = 0; i < type.memberTypes.size(); i++) {
                    dumpTypeAt(types[type.memberTypes[i]], ptr);
                    ptr += typeSizes[type.memberTypes[i]];
                    if(i < type.memberTypes.size() - 1)
                        std::cout << ", ";
                }
                std::cout << "}";
            },
        }, type);
    }

    static spv_result_t handleHeader(void* user_data, spv_endianness_t endian,
                               uint32_t /* magic */, uint32_t version,
                               uint32_t generator, uint32_t id_bound,
                               uint32_t schema)
    {
        // auto ip = static_cast<Interpreter*>(user_data);
        return SPV_SUCCESS;
    }

    static spv_result_t handleInstruction(void* user_data, const spv_parsed_instruction_t* insn)
    {
        auto ip = static_cast<Interpreter*>(user_data);

        auto opds = insn->operands;

        int which = 0;

        // Read the next uint32_t.
        auto nextu = [insn, opds, &which](uint32_t deflt = 0xFFFFFFFF) {
            return (which < insn->num_operands) ? insn->words[opds[which++].offset] : deflt;
        };

        // Read the next string.
        auto nexts = [insn, opds, &which]() {
            const char *s = reinterpret_cast<const char *>(&insn->words[opds[which].offset]);
            which++;
            return s;
        };

        // Read the next vector.
        auto nextv = [insn, opds, &which]() {
            std::vector<uint32_t> v;
            if(which < insn->num_operands) {
                v = std::vector<uint32_t> {&insn->words[opds[which].offset], &insn->words[opds[which].offset] + opds[which].num_words};
            } else {
                v = {};
            }
            which++; // XXX advance by opds[which].num_words? And move up into the "if"?
            return v;
        };

        // Read the rest of the operands as uint32_t.
        auto restv = [insn, opds, &which]() {
            std::vector<uint32_t> v;
            while(which < insn->num_operands) {
                v.push_back(insn->words[opds[which++].offset]);
            }
            return v;
        };

        switch(insn->opcode) {

            case SpvOpCapability: {
                uint32_t cap = nextu();
                assert(cap == SpvCapabilityShader);
                ip->capabilities.insert(cap);
                if(ip->verbose) {
                    std::cout << "OpCapability " << cap << " \n";
                }
                break;
            }

            case SpvOpExtInstImport: {
                // XXX result id
                uint32_t id = nextu();
                const char *name = nexts();
                assert(strcmp(name, "GLSL.std.450") == 0);
                ip->extInstSets[id] = name;
                if(ip->verbose) {
                    std::cout << "OpExtInstImport " << insn->words[opds[0].offset] << " " << name << "\n";
                }
                break;
            }

            case SpvOpMemoryModel: {
                ip->addressingModel = nextu();
                ip->memoryModel = nextu();
                assert(ip->addressingModel == SpvAddressingModelLogical);
                assert(ip->memoryModel == SpvMemoryModelGLSL450);
                if(ip->verbose) {
                    std::cout << "OpMemoryModel " << ip->addressingModel << " " << ip->memoryModel << "\n";
                }
                break;
            }

            case SpvOpEntryPoint: {
                // XXX not result id but id must eventually be Function result id
                uint32_t executionModel = nextu();
                uint32_t id = nextu();
                std::string name = nexts();
                std::vector<uint32_t> interfaceIds = restv();
                assert(executionModel == SpvExecutionModelFragment);
                ip->entryPoints[id] = {executionModel, name, interfaceIds};
                if(ip->verbose) {
                    std::cout << "OpEntryPoint " << executionModel << " " << id << " " << name;
                    for(auto& i: interfaceIds)
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpExecutionMode: {
                uint32_t entryPointId = nextu();
                uint32_t executionMode = nextu();
                std::vector<uint32_t> operands = nextv();
                ip->entryPoints[entryPointId].executionModesToOperands[executionMode] = operands;

                if(ip->verbose) {
                    std::cout << "OpExecutionMode " << entryPointId << " " << executionMode;
                    for(auto& i: ip->entryPoints[entryPointId].executionModesToOperands[executionMode])
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpString: {
                // XXX result id
                uint32_t id = nextu();
                std::string name = nexts();
                ip->strings[id] = name;
                if(ip->verbose) {
                    std::cout << "OpString " << id << " " << name << "\n";
                }
                break;
            }

            case SpvOpName: {
                uint32_t id = nextu();
                std::string name = nexts();
                ip->names[id] = name;
                if(ip->verbose) {
                    std::cout << "OpName " << id << " " << name << "\n";
                }
                break;
            }

            case SpvOpSource: {
                uint32_t language = nextu();
                uint32_t version = nextu();
                uint32_t file = nextu(SOURCE_NO_FILE);
                std::string source = (insn->num_operands > 3) ? nexts() : "";
                ip->sources.push_back({language, version, file, source});
                if(ip->verbose) {
                    std::cout << "OpSource " << language << " " << version << " " << file << " " << ((source.size() > 0) ? "with source" : "without source") << "\n";
                }
                break;
            }

            case SpvOpMemberName: {
                uint32_t type = nextu();
                uint32_t member = nextu();
                std::string name = nexts();
                ip->memberNames[type][member] = name;
                if(ip->verbose) {
                    std::cout << "OpMemberName " << type << " " << member << " " << name << "\n";
                }
                break;
            }

            case SpvOpModuleProcessed: {
                std::string process = nexts();
                ip->processes.push_back(process);
                if(ip->verbose) {
                    std::cout << "OpModulesProcessed " << process << "\n";
                }
                break;
            }

            case SpvOpDecorate: {
                uint32_t id = nextu();
                uint32_t decoration = nextu();
                std::vector<uint32_t> operands = nextv();
                ip->decorations[id] = {decoration, operands};
                if(ip->verbose) {
                    std::cout << "OpDecorate " << id << " " << decoration;
                    for(auto& i: ip->decorations[id].operands)
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpMemberDecorate: {
                uint32_t id = nextu();
                uint32_t member = nextu();
                uint32_t decoration = nextu();
                std::vector<uint32_t> operands = nextv();
                ip->memberDecorations[id][member] = {decoration, operands};
                if(ip->verbose) {
                    std::cout << "OpMemberDecorate " << id << " " << member << " " << decoration;
                    for(auto& i: ip->memberDecorations[id][member].operands)
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpTypeVoid: {
                // XXX result id
                uint32_t id = nextu();
                ip->types[id] = TypeVoid {};
                ip->typeSizes[id] = 0;
                if(ip->verbose) {
                    std::cout << "TypeVoid " << id << "\n";
                }
                break;
            }

            case SpvOpTypeFloat: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t width = nextu();
                assert(width <= 32); // XXX deal with larger later
                ip->types[id] = TypeFloat {width};
                ip->typeSizes[id] = ((width + 31) / 32) * 4;
                if(ip->verbose) {
                    std::cout << "TypeFloat " << id << " " << width << "\n";
                }
                break;
            }

            case SpvOpTypeInt: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t width = nextu();
                uint32_t signedness = nextu();
                assert(width <= 32); // XXX deal with larger later
                ip->types[id] = TypeInt {width, signedness};
                ip->typeSizes[id] = ((width + 31) / 32) * 4;
                if(ip->verbose) {
                    std::cout << "TypeInt " << id << " width " << width << " signedness " << signedness << "\n";
                }
                break;
            }

            case SpvOpTypeFunction: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t returnType = nextu();
                std::vector<uint32_t> params = restv();
                ip->types[id] = TypeFunction {returnType, params};
                ip->typeSizes[id] = 4; // XXX ?!?!?
                if(ip->verbose) {
                    std::cout << "TypeFunction " << id << " returning " << returnType;
                    if(params.size() > 1) {
                        std::cout << " with parameter types"; 
                        for(int i = 0; i < params.size(); i++)
                            std::cout << " " << params[i];
                    }
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpTypeVector: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t type = nextu();
                uint32_t count = nextu();
                ip->types[id] = TypeVector {type, count};
                ip->typeSizes[id] = ip->typeSizes[type] * count;
                if(ip->verbose) {
                    std::cout << "TypeVector " << id << " of " << type << " count " << count << "\n";
                }
                break;
            }

            case SpvOpTypePointer: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t storageClass = nextu();
                uint32_t type = nextu();
                ip->types[id] = TypePointer {type, storageClass};
                ip->typeSizes[id] = sizeof(uint32_t);
                if(ip->verbose) {
                    std::cout << "TypePointer " << id << " class " << storageClass << " type " << type << "\n";
                }
                break;
            }

            case SpvOpTypeStruct: {
                // XXX result id
                uint32_t id = nextu();
                std::vector<uint32_t> memberTypes = restv();
                ip->types[id] = TypeStruct {memberTypes};
                size_t size = 0;
                for(auto& t: memberTypes) {
                    size += ip->typeSizes[t];
                }
                ip->typeSizes[id] = size;
                if(ip->verbose) {
                    std::cout << "TypeStruct " << id;
                    if(memberTypes.size() > 0) {
                        std::cout << " members"; 
                        for(auto& i: memberTypes)
                            std::cout << " " << i;
                    }
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpVariable: {
                // XXX result id
                uint32_t type = nextu();
                uint32_t id = nextu();
                uint32_t storageClass = nextu();
                uint32_t initializer = nextu(NO_INITIALIZER);
                size_t offset = ip->allocate(storageClass, type);
                ip->variables[id] = {type, storageClass, initializer, offset};
                if(ip->verbose) {
                    std::cout << "Variable " << id << " type " << type << " storageClass " << storageClass;
                    if(initializer != NO_INITIALIZER)
                        std::cout << " initializer " << initializer;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpConstant: {
                // XXX result id
                uint32_t typeId = nextu();
                uint32_t id = nextu();
                assert(opds[2].num_words == 1); // XXX limit to 32 bits for now
                uint32_t value = nextu();
                ip->constants[id] = { typeId, value };
                if(ip->verbose) {
                    std::cout << "Constant " << id << " type " << typeId << " value " << value << "\n";
                }
                break;
            }

            case SpvOpTypeSampledImage: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t imageType = nextu();
                ip->types[id] = TypeSampledImage { imageType };
                if(ip->verbose) {
                    std::cout << "TypeSampledImage " << id
                        << " imageType " << imageType
                        << "\n";
                }
                break;
            }

            case SpvOpTypeImage: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t sampledType = nextu();
                uint32_t dim = nextu();
                uint32_t depth = nextu();
                uint32_t arrayed = nextu();
                uint32_t ms = nextu();
                uint32_t sampled = nextu();
                uint32_t imageFormat = nextu();
                uint32_t accessQualifier = nextu(NO_ACCESS_QUALIFIER);
                ip->types[id] = TypeImage { sampledType, dim, depth, arrayed, ms, sampled, imageFormat, accessQualifier };
                if(ip->verbose) {
                    std::cout << "TypeImage " << id
                        << " sampledType " << sampledType
                        << " dim " << dim
                        << " depth " << depth
                        << " arrayed " << arrayed
                        << " ms " << ms
                        << " sampled " << sampled
                        << " imageFormat " << imageFormat
                        << " accessQualifier " << accessQualifier
                        << "\n";
                }
                break;
            }

            case SpvOpFunction: {
                uint32_t resultType = nextu();
                uint32_t id = nextu();
                uint32_t functionControl = nextu();
                uint32_t functionType = nextu();
                uint32_t start = ip->code.size();
                ip->functions[id] = Function {id, resultType, functionControl, functionType, start };
                ip->currentFunction = &ip->functions[id];
                if(ip->verbose) {
                    std::cout << "Function " << id
                        << " resultType " << resultType
                        << " functionControl " << functionControl
                        << " functionType " << functionType
                        << "\n";
                }
                break;
            }

                                /*
            case SpvOpFunctionParameter: {
                assert(ip->currentFunction != nullptr);
                uint32_t type = nextu();
                uint32_t id = nextu();
                ip->code.push_back(InsnFunctionParameter{type, id});
                if(ip->verbose) {
                    std::cout << "FunctionParameter " << id
                        << " type " << type
                        << "\n";
                }
                break;
            }
            */

            case SpvOpLabel: {
                uint32_t id = nextu();
                ip->labels[id] = ip->code.size();
                if(ip->verbose) {
                    std::cout << "Label " << id
                        << " at " << ip->labels[id]
                        << "\n";
                }
                break;
            }

            case SpvOpFunctionEnd: {
                ip->currentFunction = NULL;
                if(ip->verbose) {
                    std::cout << "FunctionEnd\n";
                }
                break;
            }

            // Decode the instructions.
#include "opcode_decode.h"
            
            default: {
                if(ip->throwOnUnimplemented) {
                    throw std::runtime_error("unimplemented opcode " + OpcodeToString[insn->opcode] + " (" + std::to_string(insn->opcode) + ")");
                } else {
                    std::cout << "unimplemented opcode " << OpcodeToString[insn->opcode] << " (" << insn->opcode << ")\n";
                }
                break;
            }
        }

        return SPV_SUCCESS;
    }

    void prepare()
    {
        mainFunction = nullptr;
        for(auto& e: entryPoints) {
            if(e.second.name == "main") {
                mainFunction = &functions[e.first];
            }
        }
    }

    uint32_t pc;
    std::vector<size_t> callstack;

    void stepLoad(const InsnLoad& insn)
    {
        RegisterPointer& ptr = std::get<RegisterPointer>(registers[insn.pointerId]);
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::copy(memory + ptr.offset, memory + ptr.offset + typeSizes[insn.type], obj.data);
        if(false) {
            std::cout << "load result is";
            dumpTypeAt(types[insn.type], obj.data);
            std::cout << "\n";
        }
    }

    void stepStore(const InsnStore& insn)
    {
        RegisterPointer& ptr = std::get<RegisterPointer>(registers[insn.pointerId]);
        RegisterObject& obj = std::get<RegisterObject>(registers[insn.objectId]);
        std::copy(obj.data, obj.data + obj.size, memory + ptr.offset);
    }

    void stepCompositeExtract(const InsnCompositeExtract& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        RegisterObject& src = std::get<RegisterObject>(registers[insn.compositeId]);
        /* use indexes to walk blob */
        uint32_t type = src.type;
        size_t offset = 0;
        for(auto& j: insn.indexesId) {
            for(int i = 0; i < j; i++) {
                offset += typeSizes[getConstituentType(type, i)];
            }
            type = getConstituentType(type, j);
        }
        std::copy(src.data + offset, src.data + offset + typeSizes[obj.type], obj.data);
        if(false) {
            std::cout << "extracted from ";
            dumpTypeAt(types[src.type], src.data);
            std::cout << " result is ";
            dumpTypeAt(types[insn.type], obj.data);
            std::cout << "\n";
        }
    }

    void stepCompositeConstruct(const InsnCompositeConstruct& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        size_t offset = 0;
        for(auto& j: insn.constituentsId) {
            RegisterObject& src = std::get<RegisterObject>(registers[j]);
            std::copy(src.data, src.data + typeSizes[src.type], obj.data + offset);
            offset += typeSizes[src.type];
        }
        if(false) {
            std::cout << "constructed ";
            dumpTypeAt(types[obj.type], obj.data);
            std::cout << "\n";
        }
    }

    void stepFMul(const InsnFMul& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                float quotient = operand1 * operand2;
                registerAs<float>(insn.resultId) = quotient;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] * operand2[i];
                }

            }
        }, types[insn.type]);
    }

    void stepFDiv(const InsnFDiv& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float dividend = registerAs<float>(insn.operand1Id);
                float divisor = registerAs<float>(insn.operand2Id);
                float quotient = dividend / divisor;
                registerAs<float>(insn.resultId) = quotient;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* dividend = &registerAs<float>(insn.operand1Id);
                float* divisor = &registerAs<float>(insn.operand2Id);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = dividend[i] / divisor[i];
                }

            }
        }, types[insn.type]);
    }

    void stepConvertSToF(const InsnConvertSToF& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                int32_t src = registerAs<int32_t>(insn.signedValueId);
                registerAs<float>(insn.resultId) = src;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                int32_t* src = &registerAs<int32_t>(insn.signedValueId);
                float* dst = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    dst[i] = src[i];
                }
            }
        }, types[insn.type]);
    }

    void stepAccessChain(const InsnAccessChain& insn)
    {
        RegisterPointer& basePointer = std::get<RegisterPointer>(registers[insn.baseId]);
        uint32_t type = basePointer.type;
        size_t offset = basePointer.offset;
        for(auto& id: insn.indexesId) {
            int32_t j = registerAs<int32_t>(id);
            for(int i = 0; i < j; i++) {
                offset += typeSizes[getConstituentType(type, i)];
            }
            type = getConstituentType(type, j);
        }
        if(false) {
            std::cout << "accesschain of " << basePointer.offset << " yielded " << offset << "\n";
        }
        uint32_t pointedType = std::get<TypePointer>(types[insn.type]).type;
        registers[insn.resultId] = RegisterPointer { pointedType, basePointer.storageClass, offset };
    }

    void stepFunctionParameter(const InsnFunctionParameter& insn)
    {
        uint32_t sourceId = callstack.back();  callstack.pop_back();
        registers[insn.resultId] = registers[sourceId];
        if(false) std::cout << "function parameter " << insn.resultId << " receives " << sourceId << "\n";
    }

    void stepReturn(const InsnReturn& insn)
    {
         callstack.pop_back(); // return parameter not used.
         pc = callstack.back(); callstack.pop_back();
    }

    void stepFunctionCall(const InsnFunctionCall& insn)
    {
        const Function& function = functions[insn.functionId];

        callstack.push_back(pc);
        callstack.push_back(insn.resultId);
        for(int i = insn.operandId.size() - 1; i >= 0; i--) {
            uint32_t argument = insn.operandId[i];
            assert(std::holds_alternative<RegisterPointer>(registers[argument]));
            callstack.push_back(argument);
        }
        pc = function.start;
    }

    // Unimplemented instructions. To implement one, move the function from this
    // header into this file just above this comment.
#include "opcode_impl.h"

    void step()
    {
        if(false) std::cout << "address " << pc << "\n";

        Instruction insn = code[pc++];

        std::visit(overloaded {
#include "opcode_dispatch.h"
        }, insn);
    }

    template <class T>
    void set(SpvStorageClass clss, size_t offset, const T& v)
    {
        objectInClassAt<T>(clss, offset) = v;
    }

    template <class T>
    void get(SpvStorageClass clss, size_t offset, T& v)
    {
        v = objectInClassAt<T>(clss, offset);
    }

    void run()
    {
        registers.clear();
        // init Function variables with initializers before each invocation
        // XXX also need to initialize within function calls?
        for(auto v: variables) {
            const Variable& var = v.second;
            uint32_t pointedType = std::get<TypePointer>(types[var.type]).type;
            registers[v.first] = RegisterPointer { pointedType, var.storageClass, var.offset };
            if(v.second.storageClass == SpvStorageClassFunction) {
                assert(v.second.initializer == NO_INITIALIZER); // XXX will do initializers later
            }
        }

        for(auto c: constants) {
            const Constant& constant = c.second;
            RegisterObject& r = allocRegisterObject(c.first, constant.type);
            const unsigned char *data = reinterpret_cast<const unsigned char*>(&constant.value);
            std::copy(data, data + typeSizes[constant.type], r.data);
        }

        size_t oldTop = memoryRegions[SpvStorageClassPrivate].top;
        callstack.push_back(EXECUTION_ENDED); // caller PC
        callstack.push_back(NO_RETURN_REGISTER); // return register
        pc = mainFunction->start;

        do {
            step();
        } while(pc != EXECUTION_ENDED);

        memoryRegions[SpvStorageClassPrivate].top = oldTop;
    }
};

void eval(Interpreter& ip, float x, float y, v4float& color)
{
    if(0) {
        color[0] = x / imageWidth;
        color[1] = y / imageHeight;
        color[2] = 0.5f;
        color[3] = 1.0f;
    } else {
        ip.set(SpvStorageClassInput, 0, v2float {x, y}); // fragCoord is in #0 in preamble
        ip.run();
        ip.get(SpvStorageClassOutput, 0, color); // color is out #0 in preamble
    }
}


std::string readFileContents(std::string shaderFileName)
{
    std::ifstream shaderFile(shaderFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    std::ifstream::pos_type size = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);

    std::string text(size, '\0');
    shaderFile.read(&text[0], size);

    return text;
}

std::string readStdin()
{
    std::istreambuf_iterator<char> begin(std::cin), end;
    return std::string(begin, end);
}

void usage(const char* progname)
{
    printf("usage: %s [options] shader.frag\n", progname);
    printf("provide \"-\" as a filename to read from stdin\n");
    printf("options:\n");
    printf("\t-v    Print opcodes as they are parsed\n");
    printf("\t-g    Generate debugging information\n");
    printf("\t-O    Run optimizing passes\n");
    printf("\t-t    Throw an exception on first unimplemented opcode\n");
}

int main(int argc, char **argv)
{
    bool debug = false;
    bool optimize = false;
    bool beVerbose = false;
    bool throwOnUnimplemented = false;

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-g") == 0) {

            debug = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-v") == 0) {

            beVerbose = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-t") == 0) {

            throwOnUnimplemented = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-O") == 0) {

            optimize = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-h") == 0) {

            usage(progname);
            exit(EXIT_SUCCESS);

        } else if(strcmp(argv[0], "-") == 0) {

            // Read from stdin.
            break;

        } else {

            usage(progname);
            exit(EXIT_FAILURE);

        }
    }


    if(argc < 1) {
        usage(progname);
        exit(EXIT_FAILURE);
    }

    std::string preambleFilename = "preamble.frag";
    std::string preamble = readFileContents(preambleFilename.c_str());
    std::string epilogueFilename = "epilogue.frag";
    std::string epilogue = readFileContents(epilogueFilename.c_str());

    std::string filename;
    std::string text;
    if(strcmp(argv[0], "-") == 0) {
        filename = "stdin";
        text = readStdin();
    } else {
        filename = argv[0];
        text = readFileContents(filename.c_str());
    }

    glslang::TShader *shader = new glslang::TShader(EShLangFragment);

    {
        const char* strings[3] = { preamble.c_str(), text.c_str(), epilogue.c_str() };
        const char* names[3] = { preambleFilename.c_str(), filename.c_str(), epilogueFilename.c_str() };
        shader->setStringsWithLengthsAndNames(strings, NULL, names, 3);
    }

    shader->setEnvInput(glslang::EShSourceGlsl, EShLangFragment, glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);

    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);

    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

    EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo);

    glslang::TShader::ForbidIncluder includer;
    TBuiltInResource resources;

    resources = glslang::DefaultTBuiltInResource;

    ShInitialize();

    if (!shader->parse(&resources, 110, false, messages, includer)) {
        std::cerr << "compile failed\n";
        std::cerr << shader->getInfoLog();
        exit(EXIT_FAILURE);
    }

    std::vector<unsigned int> spirv;
    std::string warningsErrors;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;
    options.generateDebugInfo = debug;
    options.disableOptimizer = !optimize;
    options.optimizeSize = false;
    glslang::TIntermediate *shaderInterm = shader->getIntermediate();
    glslang::GlslangToSpv(*shaderInterm, spirv, &logger, &options);

    if(false) {
        FILE *fp = fopen("spirv", "wb");
        fwrite(spirv.data(), 1, spirv.size() * 4, fp);
        fclose(fp);
    }

    Interpreter ip(throwOnUnimplemented, beVerbose);
    spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
    spvBinaryParse(context, &ip, spirv.data(), spirv.size(), Interpreter::handleHeader, Interpreter::handleInstruction, nullptr);

    ip.prepare();
    ip.set(SpvStorageClassUniform, 0, v2int {imageWidth, imageHeight}); // iResolution is uniform @0 in preamble
    ip.set(SpvStorageClassUniform, 8, 0.0f); // iTime is uniform @8 in preamble
    for(int y = 0; y < imageHeight; y++)
        for(int x = 0; x < imageWidth; x++) {
            v4float color;
            eval(ip, x + 0.5f, y + 0.5f, color);
            for(int c = 0; c < 3; c++) {
                // ShaderToy clamps the color.
                int byteColor = color[c]*255.99;
                if (byteColor < 0) byteColor = 0;
                if (byteColor > 255) byteColor = 255;
                imageBuffer[imageHeight - 1 - y][x][c] = byteColor;
            }
        }

    std::ofstream imageFile("shaded.ppm", std::ios::out | std::ios::binary);
    imageFile << "P6 " << imageWidth << " " << imageHeight << " 255\n";
    imageFile.write(reinterpret_cast<const char *>(imageBuffer), sizeof(imageBuffer));
    imageFile.close();

    exit(EXIT_SUCCESS);
}
