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

struct fvec4
{
    float v[4];
    float& operator[](int b) {
        return v[b];
    }
};

std::map<uint32_t, std::string> OpcodeToString = {
    {0, "Nop"},
    {1, "Undef"},
    {2, "SourceContinued"},
    {3, "Source"},
    {4, "SourceExtension"},
    {5, "Name"},
    {6, "MemberName"},
    {7, "String"},
    {8, "Line"},
    {10, "Extension"},
    {11, "ExtInstImport"},
    {12, "ExtInst"},
    {14, "MemoryModel"},
    {15, "EntryPoint"},
    {16, "ExecutionMode"},
    {17, "Capability"},
    {19, "TypeVoid"},
    {20, "TypeBool"},
    {21, "TypeInt"},
    {22, "TypeFloat"},
    {23, "TypeVector"},
    {24, "TypeMatrix"},
    {25, "TypeImage"},
    {26, "TypeSampler"},
    {27, "TypeSampledImage"},
    {28, "TypeArray"},
    {29, "TypeRuntimeArray"},
    {30, "TypeStruct"},
    {31, "TypeOpaque"},
    {32, "TypePointer"},
    {33, "TypeFunction"},
    {34, "TypeEvent"},
    {35, "TypeDeviceEvent"},
    {36, "TypeReserveId"},
    {37, "TypeQueue"},
    {38, "TypePipe"},
    {39, "TypeForwardPointer"},
    {41, "ConstantTrue"},
    {42, "ConstantFalse"},
    {43, "Constant"},
    {44, "ConstantComposite"},
    {45, "ConstantSampler"},
    {46, "ConstantNull"},
    {48, "SpecConstantTrue"},
    {49, "SpecConstantFalse"},
    {50, "SpecConstant"},
    {51, "SpecConstantComposite"},
    {52, "SpecConstantOp"},
    {54, "Function"},
    {55, "FunctionParameter"},
    {56, "FunctionEnd"},
    {57, "FunctionCall"},
    {59, "Variable"},
    {60, "ImageTexelPointer"},
    {61, "Load"},
    {62, "Store"},
    {63, "CopyMemory"},
    {64, "CopyMemorySized"},
    {65, "AccessChain"},
    {66, "InBoundsAccessChain"},
    {67, "PtrAccessChain"},
    {68, "ArrayLength"},
    {69, "GenericPtrMemSemantics"},
    {70, "InBoundsPtrAccessChain"},
    {71, "Decorate"},
    {72, "MemberDecorate"},
    {73, "DecorationGroup"},
    {74, "GroupDecorate"},
    {75, "GroupMemberDecorate"},
    {77, "VectorExtractDynamic"},
    {78, "VectorInsertDynamic"},
    {79, "VectorShuffle"},
    {80, "CompositeConstruct"},
    {81, "CompositeExtract"},
    {82, "CompositeInsert"},
    {83, "CopyObject"},
    {84, "Transpose"},
    {86, "SampledImage"},
    {87, "ImageSampleImplicitLod"},
    {88, "ImageSampleExplicitLod"},
    {89, "ImageSampleDrefImplicitLod"},
    {90, "ImageSampleDrefExplicitLod"},
    {91, "ImageSampleProjImplicitLod"},
    {92, "ImageSampleProjExplicitLod"},
    {93, "ImageSampleProjDrefImplicitLod"},
    {94, "ImageSampleProjDrefExplicitLod"},
    {95, "ImageFetch"},
    {96, "ImageGather"},
    {97, "ImageDrefGather"},
    {98, "ImageRead"},
    {99, "ImageWrite"},
    {100, "Image"},
    {101, "ImageQueryFormat"},
    {102, "ImageQueryOrder"},
    {103, "ImageQuerySizeLod"},
    {104, "ImageQuerySize"},
    {105, "ImageQueryLod"},
    {106, "ImageQueryLevels"},
    {107, "ImageQuerySamples"},
    {109, "ConvertFToU"},
    {110, "ConvertFToS"},
    {111, "ConvertSToF"},
    {112, "ConvertUToF"},
    {113, "UConvert"},
    {114, "SConvert"},
    {115, "FConvert"},
    {116, "QuantizeToF16"},
    {117, "ConvertPtrToU"},
    {118, "SatConvertSToU"},
    {119, "SatConvertUToS"},
    {120, "ConvertUToPtr"},
    {121, "PtrCastToGeneric"},
    {122, "GenericCastToPtr"},
    {123, "GenericCastToPtrExplicit"},
    {124, "Bitcast"},
    {126, "SNegate"},
    {127, "FNegate"},
    {128, "IAdd"},
    {129, "FAdd"},
    {130, "ISub"},
    {131, "FSub"},
    {132, "IMul"},
    {133, "FMul"},
    {134, "UDiv"},
    {135, "SDiv"},
    {136, "FDiv"},
    {137, "UMod"},
    {138, "SRem"},
    {139, "SMod"},
    {140, "FRem"},
    {141, "FMod"},
    {142, "VectorTimesScalar"},
    {143, "MatrixTimesScalar"},
    {144, "VectorTimesMatrix"},
    {145, "MatrixTimesVector"},
    {146, "MatrixTimesMatrix"},
    {147, "OuterProduct"},
    {148, "Dot"},
    {149, "IAddCarry"},
    {150, "ISubBorrow"},
    {151, "UMulExtended"},
    {152, "SMulExtended"},
    {154, "Any"},
    {155, "All"},
    {156, "IsNan"},
    {157, "IsInf"},
    {158, "IsFinite"},
    {159, "IsNormal"},
    {160, "SignBitSet"},
    {161, "LessOrGreater"},
    {162, "Ordered"},
    {163, "Unordered"},
    {164, "LogicalEqual"},
    {165, "LogicalNotEqual"},
    {166, "LogicalOr"},
    {167, "LogicalAnd"},
    {168, "LogicalNot"},
    {169, "Select"},
    {170, "IEqual"},
    {171, "INotEqual"},
    {172, "UGreaterThan"},
    {173, "SGreaterThan"},
    {174, "UGreaterThanEqual"},
    {175, "SGreaterThanEqual"},
    {176, "ULessThan"},
    {177, "SLessThan"},
    {178, "ULessThanEqual"},
    {179, "SLessThanEqual"},
    {180, "FOrdEqual"},
    {181, "FUnordEqual"},
    {182, "FOrdNotEqual"},
    {183, "FUnordNotEqual"},
    {184, "FOrdLessThan"},
    {185, "FUnordLessThan"},
    {186, "FOrdGreaterThan"},
    {187, "FUnordGreaterThan"},
    {188, "FOrdLessThanEqual"},
    {189, "FUnordLessThanEqual"},
    {190, "FOrdGreaterThanEqual"},
    {191, "FUnordGreaterThanEqual"},
    {194, "ShiftRightLogical"},
    {195, "ShiftRightArithmetic"},
    {196, "ShiftLeftLogical"},
    {197, "BitwiseOr"},
    {198, "BitwiseXor"},
    {199, "BitwiseAnd"},
    {200, "Not"},
    {201, "BitFieldInsert"},
    {202, "BitFieldSExtract"},
    {203, "BitFieldUExtract"},
    {204, "BitReverse"},
    {205, "BitCount"},
    {207, "DPdx"},
    {208, "DPdy"},
    {209, "Fwidth"},
    {210, "DPdxFine"},
    {211, "DPdyFine"},
    {212, "FwidthFine"},
    {213, "DPdxCoarse"},
    {214, "DPdyCoarse"},
    {215, "FwidthCoarse"},
    {218, "EmitVertex"},
    {219, "EndPrimitive"},
    {220, "EmitStreamVertex"},
    {221, "EndStreamPrimitive"},
    {224, "ControlBarrier"},
    {225, "MemoryBarrier"},
    {227, "AtomicLoad"},
    {228, "AtomicStore"},
    {229, "AtomicExchange"},
    {230, "AtomicCompareExchange"},
    {231, "AtomicCompareExchangeWeak"},
    {232, "AtomicIIncrement"},
    {233, "AtomicIDecrement"},
    {234, "AtomicIAdd"},
    {235, "AtomicISub"},
    {236, "AtomicSMin"},
    {237, "AtomicUMin"},
    {238, "AtomicSMax"},
    {239, "AtomicUMax"},
    {240, "AtomicAnd"},
    {241, "AtomicOr"},
    {242, "AtomicXor"},
    {245, "Phi"},
    {246, "LoopMerge"},
    {247, "SelectionMerge"},
    {248, "Label"},
    {249, "Branch"},
    {250, "BranchConditional"},
    {251, "Switch"},
    {252, "Kill"},
    {253, "Return"},
    {254, "ReturnValue"},
    {255, "Unreachable"},
    {256, "LifetimeStart"},
    {257, "LifetimeStop"},
    {259, "GroupAsyncCopy"},
    {260, "GroupWaitEvents"},
    {261, "GroupAll"},
    {262, "GroupAny"},
    {263, "GroupBroadcast"},
    {264, "GroupIAdd"},
    {265, "GroupFAdd"},
    {266, "GroupFMin"},
    {267, "GroupUMin"},
    {268, "GroupSMin"},
    {269, "GroupFMax"},
    {270, "GroupUMax"},
    {271, "GroupSMax"},
    {274, "ReadPipe"},
    {275, "WritePipe"},
    {276, "ReservedReadPipe"},
    {277, "ReservedWritePipe"},
    {278, "ReserveReadPipePackets"},
    {279, "ReserveWritePipePackets"},
    {280, "CommitReadPipe"},
    {281, "CommitWritePipe"},
    {282, "IsValidReserveId"},
    {283, "GetNumPipePackets"},
    {284, "GetMaxPipePackets"},
    {285, "GroupReserveReadPipePackets"},
    {286, "GroupReserveWritePipePackets"},
    {287, "GroupCommitReadPipe"},
    {288, "GroupCommitWritePipe"},
    {291, "EnqueueMarker"},
    {292, "EnqueueKernel"},
    {293, "GetKernelNDrangeSubGroupCount"},
    {294, "GetKernelNDrangeMaxSubGroupSize"},
    {295, "GetKernelWorkGroupSize"},
    {296, "GetKernelPreferredWorkGroupSizeMultiple"},
    {297, "RetainEvent"},
    {298, "ReleaseEvent"},
    {299, "CreateUserEvent"},
    {300, "IsValidEvent"},
    {301, "SetUserEventStatus"},
    {302, "CaptureEventProfilingInfo"},
    {303, "GetDefaultQueue"},
    {304, "BuildNDRange"},
    {305, "ImageSparseSampleImplicitLod"},
    {306, "ImageSparseSampleExplicitLod"},
    {307, "ImageSparseSampleDrefImplicitLod"},
    {308, "ImageSparseSampleDrefExplicitLod"},
    {309, "ImageSparseSampleProjImplicitLod"},
    {310, "ImageSparseSampleProjExplicitLod"},
    {311, "ImageSparseSampleProjDrefImplicitLod"},
    {312, "ImageSparseSampleProjDrefExplicitLod"},
    {313, "ImageSparseFetch"},
    {314, "ImageSparseGather"},
    {315, "ImageSparseDrefGather"},
    {316, "ImageSparseTexelsResident"},
    {317, "NoLine"},
    {318, "AtomicFlagTestAndSet"},
    {319, "AtomicFlagClear"},
    {320, "ImageSparseRead"},
    {321, "SizeOf"},
    {322, "TypePipeStorage"},
    {323, "ConstantPipeStorage"},
    {324, "CreatePipeFromPipeStorage"},
    {325, "GetKernelLocalSizeForSubgroupCount"},
    {326, "GetKernelMaxNumSubgroups"},
    {327, "TypeNamedBarrier"},
    {328, "NamedBarrierInitialize"},
    {329, "MemoryNamedBarrier"},
    {330, "ModuleProcessed"},
    {331, "ExecutionModeId"},
    {332, "DecorateId"},
    {4421, "SubgroupBallotKHR"},
    {4422, "SubgroupFirstInvocationKHR"},
    {4428, "SubgroupAllKHR"},
    {4429, "SubgroupAnyKHR"},
    {4430, "SubgroupAllEqualKHR"},
    {4432, "SubgroupReadInvocationKHR"},
    {5000, "GroupIAddNonUniformAMD"},
    {5001, "GroupFAddNonUniformAMD"},
    {5002, "GroupFMinNonUniformAMD"},
    {5003, "GroupUMinNonUniformAMD"},
    {5004, "GroupSMinNonUniformAMD"},
    {5005, "GroupFMaxNonUniformAMD"},
    {5006, "GroupUMaxNonUniformAMD"},
    {5007, "GroupSMaxNonUniformAMD"},
    {5011, "FragmentMaskFetchAMD"},
    {5012, "FragmentFetchAMD"},
    {5571, "SubgroupShuffleINTEL"},
    {5572, "SubgroupShuffleDownINTEL"},
    {5573, "SubgroupShuffleUpINTEL"},
    {5574, "SubgroupShuffleXorINTEL"},
    {5575, "SubgroupBlockReadINTEL"},
    {5576, "SubgroupBlockWriteINTEL"},
    {5577, "SubgroupImageBlockReadINTEL"},
    {5578, "SubgroupImageBlockWriteINTEL"},
    {5632, "DecorateStringGOOGLE"},
    {5633, "MemberDecorateStringGOOGLE"},
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

struct Variable
{
    uint32_t type;
    uint32_t storageClass;
    uint32_t initializer;
    size_t offset;
};

struct EntryPoint
{
    uint32_t executionModel;
    std::string name;
    std::vector<uint32_t> interfaceIds;
    std::map<uint32_t, std::vector<uint32_t>> executionModesToOperands;
};

struct Decoration
{
    uint32_t decoration;
    std::vector<uint32_t> operands;
};

struct Source
{
    uint32_t language;
    uint32_t version;
    uint32_t file;
    std::string source;
};

struct MemberName
{
    uint32_t type;
    uint32_t member;
    std::string name;
};

struct TypeVoid
{
    int foo;
};

struct TypeInt
{
    uint32_t width;
    uint32_t signedness;
};

struct TypeFloat
{
    uint32_t width;
};

struct TypeVector
{
    uint32_t type;
    uint32_t count;
};

struct TypeFunction
{
    uint32_t returnType;
    std::vector<uint32_t> parameterTypes;
};

struct TypeStruct
{
    std::vector<uint32_t> memberTypes;
};

struct TypePointer
{
    uint32_t type;
    uint32_t storageClass;
};

struct TypeImage
{
    uint32_t sampledType;
    uint32_t dim;
    uint32_t depth;
    uint32_t arrayed;
    uint32_t ms;
    uint32_t sampled;
    uint32_t imageFormat;
    uint32_t accessQualifier;
};

struct TypeSampledImage
{
    uint32_t imageType;
};

typedef std::variant<TypeVoid, TypeFloat, TypePointer, TypeFunction, TypeVector, TypeInt, TypeStruct, TypeImage, TypeSampledImage> Type;

struct Constant
{
    uint32_t type;
    uint32_t value;
};

struct FunctionParameter
{
    uint32_t type;
    uint32_t id;
};

struct InsnLoad {
    uint32_t type;
    uint32_t resultId;
    uint32_t pointerId;
    uint32_t memoryAccess;
};

struct InsnStore {
    uint32_t pointerId;
    uint32_t objectId;
    uint32_t memoryAccess;
};

struct InsnAccessChain {
    uint32_t type;
    uint32_t resultId;
    uint32_t basePointerId;
    std::vector<uint32_t> indexes;
};

struct InsnCompositeConstruct {
    uint32_t type;
    uint32_t resultId;
    std::vector<uint32_t> constituentIds;
};

struct InsnCompositeExtract {
    uint32_t type;
    uint32_t resultId;
    uint32_t compositeId;
    std::vector<uint32_t> indexes;
};

struct InsnConvertSToF {
    uint32_t type;
    uint32_t resultId;
    uint32_t sourceId;
};

struct InsnFDiv {
    uint32_t type;
    uint32_t resultId;
    uint32_t operand1Id; // dividend
    uint32_t operand2Id; // divisor
};

struct InsnFunctionCall {
    uint32_t type;
    uint32_t resultId;
    uint32_t functionId;
    std::vector<uint32_t> argumentIds;
};

struct InsnReturn {
};

const uint32_t NO_MEMORY_ACCESS_SEMANTIC = 0xFFFFFFFF;

typedef std::variant<InsnLoad, InsnStore, InsnAccessChain, InsnCompositeConstruct, InsnCompositeExtract, InsnConvertSToF, InsnFDiv, InsnFunctionCall, InsnReturn> Instruction;

struct Function {
    uint32_t id;
    uint32_t resultType;
    uint32_t functionControl;
    uint32_t functionType;
    std::vector<FunctionParameter> parameters;
    std::vector<Instruction> code;
};

struct Label {
    Function *function;
    size_t instruction;
};

typedef std::array<float,4> vec4f;
typedef std::array<uint32_t,4> vec4u;
typedef std::array<int32_t,4> vec4i;

const uint32_t SOURCE_NO_FILE = 0xFFFFFFFF;
const uint32_t NO_INITIALIZER = 0xFFFFFFFF;
const uint32_t NO_ACCESS_QUALIFIER = 0xFFFFFFFF;

struct InstructionAddress
{
    Function *function;
    uint32_t address;
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
    std::map<uint32_t, Variable> variables;
    std::map<uint32_t, size_t> intermediates;
    std::map<uint32_t, Constant> constants;
    std::map<uint32_t, Function> functions;
    Function* currentFunction;
    std::map<uint32_t, Label> labels;
    Function* mainFunction; 

    unsigned char memory[65536];
    size_t memoryTop;

    size_t allocate(uint32_t type)
    {
        assert(memoryTop + typeSizes[type] <= sizeof(memory));
        size_t offset = memoryTop;
        memoryTop += typeSizes[type];
        return offset;
    }

    void copy(uint32_t type, size_t src, size_t dst)
    {
        std::copy(memory + src, memory + src + typeSizes[type], memory + dst);
    }


    Interpreter(bool throwOnUnimplemented_, bool verbose_) :
        throwOnUnimplemented(throwOnUnimplemented_),
        verbose(verbose_),
        currentFunction(nullptr),
        memoryTop(0)
    { }

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

        auto nextu = [insn, opds, &which](uint32_t deflt = 0xFFFFFFFF) {
            return (which < insn->num_operands) ? insn->words[opds[which++].offset] : deflt;
        };

        auto nexts = [insn, opds, &which]() {
            const char *s = reinterpret_cast<const char *>(&insn->words[opds[which].offset]);
            which++;
            return s;
        };

        auto nextv = [insn, opds, &which]() {
            std::vector<uint32_t> v;
            if(which < insn->num_operands) {
                v = std::vector<uint32_t> {&insn->words[opds[which].offset], &insn->words[opds[which].offset] + opds[which].num_words};
            } else {
                v = {};
            }
            which++;
            return v;
        };

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
                ip->typeSizes[id] = (((width + 31) / 32) * 32) * 4;
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
                ip->typeSizes[id] = (((width + 31) / 32) * 32) * 4;
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
                        std::cout << " with parameters"; 
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
                size_t offset = ip->allocate(type);
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
                std::cout << "Constant " << id << " type " << typeId << " value " << value << "\n";
                break;
            }

            case SpvOpTypeSampledImage: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t imageType = nextu();
                ip->types[id] = TypeSampledImage { imageType };
                std::cout << "TypeSampledImage " << id
                    << " imageType " << imageType
                    << "\n";
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
                break;
            }

            case SpvOpFunction: {
                uint32_t resultType = nextu();
                uint32_t id = nextu();
                uint32_t functionControl = nextu();
                uint32_t functionType = nextu();
                ip->functions[id] = Function {id, resultType, functionControl, functionType };
                ip->currentFunction = &ip->functions[id];
                std::cout << "Function " << id
                    << " resultType " << resultType
                    << " functionControl " << functionControl
                    << " functionType " << functionType
                    << "\n";
                break;
            }

            case SpvOpFunctionParameter: {
                assert(ip->currentFunction != nullptr);
                uint32_t type = nextu();
                uint32_t id = nextu();
                ip->currentFunction->parameters.push_back({type, id});
                std::cout << "FunctionParameter " << id
                    << " type " << type
                    << "\n";
                break;
            }

            case SpvOpLabel: {
                uint32_t id = nextu();
                ip->labels[id] = {ip->currentFunction, ip->currentFunction->code.size()};
                std::cout << "Label " << id
                    << " is Function " << ip->labels[id].function->id
                    << " at " << ip->labels[id].instruction
                    << "\n";
                break;
            }

            case SpvOpFunctionEnd: {
                ip->currentFunction = NULL;
                std::cout << "FunctionEnd\n";
                break;
            }

            case SpvOpReturn: {
                ip->currentFunction->code.push_back(InsnReturn{});
                std::cout << "Return\n";
                break;
            }

            case SpvOpLoad: {
                uint32_t type = nextu();
                uint32_t id = nextu();
                uint32_t pointer = nextu();
                uint32_t memoryAccess = nextu(NO_MEMORY_ACCESS_SEMANTIC);
                ip->currentFunction->code.push_back(InsnLoad{type, id, pointer, memoryAccess});
                std::cout << "Load"
                    << " type " << type
                    << " id " << id
                    << " pointer " << pointer;
                if(memoryAccess != NO_MEMORY_ACCESS_SEMANTIC)
                    std::cout << " type " << memoryAccess;
                std::cout << "\n";
                break;
            }

            case SpvOpStore: {
                uint32_t pointer = nextu();
                uint32_t object = nextu();
                uint32_t memoryAccess = nextu(NO_MEMORY_ACCESS_SEMANTIC);
                ip->currentFunction->code.push_back(InsnStore{pointer, object, memoryAccess});
                std::cout << "Store"
                    << " pointer " << pointer
                    << " object " << object;
                if(memoryAccess != NO_MEMORY_ACCESS_SEMANTIC)
                    std::cout << " type " << memoryAccess;
                std::cout << "\n";
                break;
            }
            
            case SpvOpAccessChain: {
                uint32_t type = nextu();
                uint32_t resultId = nextu();
                uint32_t basePointerId = nextu();
                auto indexes = restv();
                ip->currentFunction->code.push_back(InsnAccessChain{type, resultId, basePointerId, indexes});
                std::cout << "AccessChain"
                    << " type " << type
                    << " resultId " << resultId
                    << " basePointerId " << basePointerId;
                std::cout << " indexes"; 
                for(int i = 0; i < indexes.size(); i++)
                    std::cout << " " << indexes[i];
                std::cout << "\n";
                break;
            }
            
            case SpvOpCompositeConstruct: {
                uint32_t type = nextu();
                uint32_t resultId = nextu();
                auto constituentIds = restv();
                ip->currentFunction->code.push_back(InsnCompositeConstruct{type, resultId, constituentIds});
                std::cout << "CompositeConstruct"
                    << " type " << type
                    << " resultId " << resultId;
                std::cout << " constituentIds"; 
                for(int i = 0; i < constituentIds.size(); i++)
                    std::cout << " " << constituentIds[i];
                std::cout << "\n";
                break;
            }
            
            case SpvOpCompositeExtract: {
                uint32_t type = nextu();
                uint32_t resultId = nextu();
                uint32_t compositeId = nextu();
                auto indexes = restv();
                ip->currentFunction->code.push_back(InsnCompositeExtract{type, resultId, compositeId, indexes});
                std::cout << "CompositeExtract"
                    << " type " << type
                    << " resultId " << resultId
                    << " compositeId " << compositeId;
                std::cout << " indexes"; 
                for(int i = 0; i < indexes.size(); i++)
                    std::cout << " " << indexes[i];
                std::cout << "\n";
                break;
            }
            
            case SpvOpConvertSToF: {
                uint32_t type = nextu();
                uint32_t resultId = nextu();
                uint32_t sourceId = nextu();
                ip->currentFunction->code.push_back(InsnConvertSToF{type, resultId, sourceId});
                std::cout << "ConvertSToF"
                    << " type " << type
                    << " resultId " << resultId
                    << " sourceId " << sourceId
                    << "\n";
                break;
            }

            case SpvOpFDiv: {
                uint32_t type = nextu();
                uint32_t resultId = nextu();
                uint32_t operand1Id = nextu();
                uint32_t operand2Id = nextu();
                ip->currentFunction->code.push_back(InsnFDiv{type, resultId, operand1Id, operand2Id});
                std::cout << "FDiv"
                    << " type " << type
                    << " resultId " << resultId
                    << " operand2Id " << operand2Id
                    << "\n";
                break;
            }

            case SpvOpFunctionCall: {
                uint32_t type = nextu();
                uint32_t resultId = nextu();
                uint32_t functionId = nextu();
                auto argumentIds = restv();
                ip->currentFunction->code.push_back(InsnFunctionCall{type, resultId, functionId, argumentIds});
                std::cout << "FunctionCall"
                    << " type " << type
                    << " resultId " << resultId
                    << " functionId " << functionId;
                std::cout << " argumentIds"; 
                for(int i = 0; i < argumentIds.size(); i++)
                    std::cout << " " << argumentIds[i];
                std::cout << "\n";
                break;
            }
            
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

    InstructionAddress pc;
    std::vector<InstructionAddress> callstack;

    void stepLoad(const InsnLoad& insn)
    {
        Variable& var = variables[insn.pointerId];
        intermediates[insn.resultId] = allocate(insn.type);
        copy(insn.type, variables[insn.pointerId].offset, intermediates[insn.resultId]);
    }

    void stepStore(const InsnStore& insn)
    {
        Variable& var = variables[insn.pointerId];
        copy(variables[insn.pointerId].type, intermediates[insn.objectId], variables[insn.pointerId].offset);
    }

    void stepCompositeExtract(const InsnCompositeExtract& insn)
    {
        intermediates[insn.resultId] = allocate(insn.type);
        /* use indexes to walk blob */
    }

    void stepCompositeConstruct(const InsnCompositeConstruct& insn)
    {
        intermediates[insn.resultId] = allocate(insn.type);
        /* use constituents to walk blob */
    }

    void stepFDiv(const InsnFDiv& insn)
    {
#if 0
        intermediates[insn.resultId] = allocate(insn.type);
        float* op1 = reinterpret_cast<float*>(memory + intermediates[insn.operand1Id]);
        float* op2 = reinterpret_cast<float*>(memory + intermediates[insn.operand2Id]);
        std::visit(overloaded {
            [&](const TypeFloat& type) { if(callstack.size() == 0) { executionDone = true; } else { pc = callstack.back(); callstack.pop_back(); } } ,
            [&](const InsnLoad& insn) { stepLoad(insn); },
            [&](const InsnStore& insn) { stepStore(insn); },
            [&](const InsnAccessChain& insn) { stepAccessChain(insn); },
            [&](const InsnCompositeConstruct& insn) { stepCompositeConstruct(insn); },
            [&](const InsnCompositeExtract& insn) { stepCompositeExtract(insn); },
            [&](const InsnConvertSToF& insn) { stepConvertSToF(insn); },
            [&](const InsnFDiv& insn) { stepFDiv(insn); },
            [&](const InsnFunctionCall& insn) { stepFunctionCall(insn); },
        }, types[insn.type]);
#endif
        /* can assume scalar or vector */
        /* divide op1 by op2, store in result */
    }

    void stepConvertSToF(const InsnConvertSToF& insn)
    {
        intermediates[insn.resultId] = allocate(insn.type);
        /* can assume scalar or vector of float */
        /* convert signed int to float */
    }

    void stepAccessChain(const InsnAccessChain& insn)
    {
        intermediates[insn.resultId] = allocate(insn.type);
        /* walk from base through children */
    }

    void stepFunctionCall(const InsnFunctionCall& insn)
    {
        intermediates[insn.resultId] = allocate(insn.type);
        /* copy call parameters to function parameter inputs */
        /* push current pc on callstack */
        /* set pc to new function and 0 */
    }

    bool step()
    {
        std::cout << "address " << pc.address << "\n";
        Instruction insn = pc.function->code[pc.address++];
        bool executionDone = false;
        std::visit(overloaded {
            [&](const InsnReturn& insn) { if(callstack.size() == 0) { executionDone = true; } else { pc = callstack.back(); callstack.pop_back(); } } ,
            [&](const InsnLoad& insn) { stepLoad(insn); },
            [&](const InsnStore& insn) { stepStore(insn); },
            [&](const InsnAccessChain& insn) { stepAccessChain(insn); },
            [&](const InsnCompositeConstruct& insn) { stepCompositeConstruct(insn); },
            [&](const InsnCompositeExtract& insn) { stepCompositeExtract(insn); },
            [&](const InsnConvertSToF& insn) { stepConvertSToF(insn); },
            [&](const InsnFDiv& insn) { stepFDiv(insn); },
            [&](const InsnFunctionCall& insn) { stepFunctionCall(insn); },
        }, insn);
        return !executionDone;
    }

    void run()
    {
        for(auto v: variables) {
            if(v.second.storageClass == SpvStorageClassFunction) {
                assert(v.second.initializer == NO_INITIALIZER); // XXX will do initializers later
            }
        }

        size_t oldTop = memoryTop;

        pc = { mainFunction, 0 };

        while(step());

        memoryTop = oldTop;
    }
};

void eval(Interpreter& ip, float u, float v, fvec4& color)
{
    if(0) {
        color[0] = u;
        color[1] = v;
        color[2] = 0.5f;
        color[3] = 1.0f;
    } else {
        /* set pipeline vars */
        ip.run();
        /* get fragcolor */
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
    printf("\t-g    Generate debugging information\n");
    printf("\t-O    Run optimizing passes\n");
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
    /* set uniforms */
    for(int y = 0; y < imageHeight; y++)
        for(int x = 0; x < imageWidth; x++) {
            fvec4 color;
            float u = (x + .5) / imageWidth;
            float v = (y + .5) / imageHeight;
            eval(ip, u, v, color);
            for(int c = 0; c < 3; c++) {
                imageBuffer[y][x][c] = color[c] * 255;
            }
        }

    std::ofstream imageFile("shaded.ppm", std::ios::out | std::ios::binary);
    imageFile << "P6 " << imageWidth << " " << imageHeight << " 255\n";
    imageFile.write(reinterpret_cast<const char *>(imageBuffer), sizeof(imageBuffer));

    exit(EXIT_SUCCESS);
}
