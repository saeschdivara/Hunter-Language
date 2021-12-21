#include "BuiltinFeatureGenerator.h"

namespace Hunter::Compiler {

    void BuiltinFeatureGenerator::GenerateBuiltinFeatures(llvm::IRBuilder<> *builder) {
        GenerateListFeature(builder);
    }

    void BuiltinFeatureGenerator::GenerateListFeature(llvm::IRBuilder<> *builder) {
        std::vector<llvm::Type *> structTypes = {
            builder->getInt64Ty(), // elements number
            llvm::PointerType::get(builder->getVoidTy(), 0) // memory
        };

        auto * structType = llvm::StructType::create(m_Generator->m_Context, structTypes, "list");

        m_Generator->m_Structs["list"] = structType;
    }
}
