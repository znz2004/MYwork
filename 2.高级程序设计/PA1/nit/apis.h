#ifndef __APIS_H__
#define __APIS_H__
#include <string>
#include <vector>
namespace UsefulApi {
    // 你不需要理解函数的实现，只需要阅读注释并且理解函数的用法
    // 你也可以自行实现相关的功能
   
    /**
    * @brief 获取当前工作目录的绝对路径。
    *
    * 此函数返回当前工作目录的完整路径。这对于需要知道程序运行时所在目录的应用程序非常有用。
    *
    * @return 返回当前工作目录的绝对路径字符串。
    */
    std::string cwd();

    /**
    * @brief 获取指定目录下的所有文件名。
    *
    * 此函数遍历指定的目录，并返回其中所有文件的名字。注意，该函数不会递归地列出子目录中的文件，列出的文件名也不包括子目录名。
    *
    * @param directoryPath 指定要列出文件的目录路径。
    * @return 返回一个包含目录中所有文件名的字符串向量。如果目录不存在或者无法访问，则可能返回一个空向量。
    */
    std::vector<std::string> listFilesInDirectory(const std::string& directoryPath);

    /**
    * @brief 将提供的字符串内容写入指定路径的文件中。
    *
    * 此函数会将提供的字符串内容写入指定路径的文件。如果文件已存在，
    * 其原有内容将被清除。如果文件不存在，则会创建一个新的文件来保存内容。
    *
    * @param str 要写入文件的字符串内容。
    * @param filePath 目标文件的路径。
    * @return 返回true表示写入成功；返回false表示写入失败（例如，权限问题）。
    *
    * 示例：
    * @code
    * bool result = UsefulApi::writeToFile("hello", "hello/world.txt");
    * if (result) {
    *     std::cout << "文件写入成功。" << std::endl;
    * } else {
    *     std::cout << "文件写入失败。" << std::endl;
    * }
    * @endcode
    */
    bool writeToFile(const std::string str, const std::string filePath);

    /**
    * @brief 从指定路径的文件中读取所有内容，并将其存储到一个字符串变量中。
    *
    * 这个函数尝试打开并读取指定路径下的文件。如果文件存在并且可以被正确打开，
    * 则会读取文件中的所有内容，并将内容存储到destStr字符串中。读取完成后，
    * 文件将被关闭。
    *
    * @param filePath 要读取的文件的路径。
    * @param destStr 用于存储文件内容的目标字符串。
    * @return 返回true表示文件读取成功；返回false表示文件读取失败（例如，文件不存在或无法打开）。
    */
    bool readFromFile(const std::string filePath, std::string& destStr);

    /**
    * @brief 计算给定字符串的 SHA-1 哈希值。
    *
    * 此函数接收一个字符串，并计算其 SHA-1 哈希值。
    *
    * @param str 需要计算哈希值的输入字符串。
    * @return 返回计算得到的 SHA-1 哈希值字符串表示形式。
    *
    */
    std::string hash(const std::string str);
};
#endif