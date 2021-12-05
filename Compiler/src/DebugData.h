#pragma once

#include <string>

namespace Hunter::Parser::Debug {
    class DebugEntry {
    public:
    };

    class DebugData {
    public:
        std::string GetDirectory() const { return m_Directory; }
        std::string GetFileName() const { return m_FileName; }
        int64_t GetFileLine() const { return m_Line; }
        int64_t GetLineColumn() const { return m_Column; }

        void SetDirectory(const std::string & directory) { m_Directory = directory; }
        void SetFileName(const std::string & fileName) { m_FileName = fileName; }
        void SetFileLine(int64_t line) { m_Line = line; }
        void SetFileColumn(int64_t column) { m_Column = column; }

    private:
        std::string m_Directory;
        std::string m_FileName;
        int64_t m_Line;
        int64_t m_Column;
    };
}