#pragma once
#include <Arduino.h>
#include <FS.h>
#include <stdint.h>
#include <stddef.h>

class Storage
{
public:
  /**
   * @struct FsInfo
   * @brief 文件系统信息结构体
   * 
   * 用于存储文件系统的容量和使用情况信息
   * 
   * @member totalBytes 文件系统总容量（字节数）
   * @member usedBytes 文件系统已使用的容量（字节数）
   */
  struct FsInfo
  {
    size_t totalBytes = 0;
    size_t usedBytes = 0;
  };

  /**
   * @brief 初始化存储系统
   * @details 初始化存储模块，为后续的读写操作做准备。必须在使用其他存储函数之前调用。
   * @return true 如果初始化成功，false 如果初始化失败
   */
  static bool begin();

  /**
   * @brief 获取文件系统信息
   * 
   * 该静态方法用于获取当前文件系统的相关信息，包括存储容量、
   * 已用空间、可用空间等详细信息。
   * 
   * @return FsInfo 包含文件系统信息的结构体，包括：
   *         - 总容量
   *         - 已用空间
   *         - 可用空间
   *         - 其他文件系统统计数据
   * 
   * @note 这是一个静态方法，可以直接通过类名调用，无需实例化对象
   */
  static FsInfo info();

  /**
   * @brief 检查指定路径的文件或目录是否存在
   * 
   * @param path 要检查的文件或目录路径
   * @return true 如果指定路径存在，返回 true
   * @return false 如果指定路径不存在，返回 false
   */
  static bool exists(const String &path);

  /**
   * @brief 删除指定路径的文件或目录
   * 
   * @param path 要删除的文件或目录的路径
   * @return true 如果删除成功，返回true
   * @return false 如果删除失败，返回false
   */
  static bool remove(const String &path);

  /**
   * @brief 确保指定路径的所有目录存在
   * 
   * 该函数检查给定路径中的所有目录是否存在，如果不存在则创建它们。
   * 这是一个递归操作，会创建路径中缺失的所有中间目录。
   * 
   * @param path 要确保存在的目录路径，可以是绝对路径或相对路径
   * 
   * @return bool 如果操作成功（所有目录已存在或已成功创建），返回 true；
   *              如果操作失败（如权限不足、磁盘空间不足等），返回 false
   * 
   * @note 该函数是静态方法，无需创建对象实例即可调用
   * 
   * @see 相关函数：删除目录、检查目录是否存在等
   */
  static bool ensureDirs(const String &path);

  class FileWriter
  {
  public:
    /**
     * @brief FileWriter 默认构造函数
     * @details 使用默认的构造函数初始化 FileWriter 对象。
     *          该构造函数不进行任何特殊的初始化操作。
     */
    FileWriter() = default;

    /**
     * @brief FileWriter 类的析构函数
     * 
     * 负责释放 FileWriter 对象占用的资源,包括关闭打开的文件句柄、
     * 释放动态分配的内存等清理工作。
     */
    ~FileWriter();

    /**
     * @brief 删除拷贝构造函数
     * 
     * 禁止通过拷贝构造函数创建 FileWriter 对象的副本。
     * 这确保了 FileWriter 对象的唯一性，防止多个对象指向同一文件资源，
     * 避免潜在的资源管理冲突和数据竞争问题。
     */
    FileWriter(const FileWriter &) = delete;

    /// @brief 删除拷贝赋值操作符
    /// @details 禁止通过赋值操作符进行对象拷贝，确保FileWriter对象不能被复制赋值，
    ///          这对于单独拥有资源（如文件句柄）的对象是必要的
    FileWriter &operator=(const FileWriter &) = delete;

    /// @brief 删除FileWriter的移动构造函数
    /// @details 禁止通过移动语义创建FileWriter对象的副本，确保FileWriter对象的生命周期管理更加严格和安全
    FileWriter(FileWriter &&) = delete;

    /// @brief 删除移动赋值操作符
    /// @details 禁止FileWriter对象进行移动赋值操作，确保对象的生命周期管理更加安全可控。
    /// 移动赋值被显式删除，防止资源所有权的转移可能带来的问题。
    FileWriter &operator=(FileWriter &&) = delete;

    /**
     * @brief 打开存储设备
     * @param path 存储设备的路径
     * @param total_size 存储设备的总大小（字节）
     * @return true 打开成功，false 打开失败
     */
    bool open(const String &path, uint32_t total_size);

    /**
     * @brief 在指定偏移量处写入数据
     * @param offset 写入的起始偏移量（字节）
     * @param data 指向待写入数据的指针
     * @param len 待写入数据的长度（字节）
     * @return true 写入成功，false 写入失败
     */
    bool writeAt(uint32_t offset, const uint8_t *data, uint16_t len);

    /**
     * @brief 关闭存储连接
     * 
     * @param commit 是否提交更改。如果为 true，则在关闭前提交所有待处理的更改；
     *               如果为 false，则放弃所有未提交的更改
     */
    void close(bool commit);

    /**
     * @brief 获取存储的总大小
     * 
     * @return uint32_t 返回存储的总大小，单位为字节
     */
    uint32_t totalSize() const;

    /**
     * @brief 获取已写入的字节数
     * @return uint32_t 返回已写入的字节数
     */
    uint32_t bytesWritten() const;

  private:

    /**
     * @brief 最终路径字符串
     * 
     * 存储经过处理或解析后的最终文件路径，用于文件操作或数据存储的目的地指定。
     */
    String finalPath_;

    /// @brief 临时路径字符串
    /// 
    /// 用于存储临时文件或临时数据的路径信息
    String tempPath_;

    /**
     * @brief 文件对象，用于存储和管理文件I/O操作
     * 
     * 该成员变量封装了文件的读写操作，提供对文件系统的访问接口。
     */
    File file_;
    
    /// @brief 存储区域总大小
    /// @details 表示存储设备或缓冲区的总容量，单位为字节
    uint32_t totalSize_ = 0;

    /**
     * @brief 已写入的字节数
     * 
     * 用于跟踪已写入存储设备的字节总数。
     */
    uint32_t bytesWritten_ = 0;

    /// @brief 存储是否已打开的标志
    /// 
    /// 用于跟踪存储设备的打开状态。当存储设备成功打开时设置为 true，
    /// 关闭时设置为 false。
    bool opened_ = false;
  };
};
