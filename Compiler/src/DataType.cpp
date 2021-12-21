#include "DataType.h"
#include "utils/logger.h"

namespace Hunter::Compiler {

    DataType *DataType::FromString(const std::string &typeStr) {

        if (typeStr == "string") {
            return new DataType(DataTypeId::String);
        }
        else if (typeStr == "memory") {
            return new DataType(DataTypeId::Memory);
        }
        else if (typeStr == "i8") {
            return new DataType(DataTypeId::i8);
        }
        else if (typeStr == "i16") {
            return new DataType(DataTypeId::i16);
        }
        else if (typeStr == "i32") {
            return new DataType(DataTypeId::i32);
        }
        else if (typeStr == "i64") {
            return new DataType(DataTypeId::i64);
        }
        else if (typeStr.empty()) {
            return new DataType(DataTypeId::Void);
        }
        else if (typeStr.starts_with("list<")) {
            std::string listSearchStr = "list<";
            std::string templateTypeStr = typeStr.substr(listSearchStr.size(), typeStr.size()-listSearchStr.size()-1);
            return new DataType(DataTypeId::List, FromString(templateTypeStr));
        }
        else {
            return new DataType(DataTypeId::Unknown);
        }

    }

}