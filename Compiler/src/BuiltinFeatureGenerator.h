#pragma once

#include "CodeGenerator.h"

namespace Hunter::Compiler {

    class BuiltinFeatureGenerator {
    public:
        BuiltinFeatureGenerator(CodeGenerator * generator) : m_Generator(generator) {}

        void GenerateBuiltinFeatures(llvm::IRBuilder<> *builder);

    protected:
        void GenerateListFeature(llvm::IRBuilder<> *builder);

    private:
        CodeGenerator * m_Generator;
    };
}
