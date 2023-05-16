#include <filesystem>
#include <iostream>
#include <mutex>
#include <fstream>
#include <unordered_map>
#include <thread>

namespace TestTask {
    struct File {
        std::string name;
        std::fstream fileStream;
    };
    struct IVFS {
        virtual File* Open(const char* name) = 0;
        virtual File* Create(const char* name) = 0;
        virtual size_t Read(File* file, char* buffer, size_t length) = 0;
        virtual size_t Write(File* file, const char* buffer, size_t length) = 0;
        virtual void Close(File* file) = 0;
        virtual ~IVFS() {}
    };
    class VFS : public IVFS {
        std::unordered_map<std::string, File*> files_;
        std::mutex mutex_;

    public:
        File* Open(const char* name) override {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = files_.find(name);
            if (it != files_.end()) {
                File* file = it->second;
                if (file->fileStream.is_open()) {
                    return nullptr;
                }
                else {
                    file->fileStream.open(name, std::ios::in);
                    if (file->fileStream.is_open()) {
                        return file;
                    } else {
                        return nullptr;
                    }
                }
            } else {
                return nullptr;
            }
        }

        File* Create(const char* name) override {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = files_.find(name);
            if (it == files_.end()) {
                File* file = new File;
                file->fileStream.open(name, std::ios::out);
                file->name = name;
                CreateDirectories(name);
                files_[name] = file;
                return file;
            } else {
                File* file = it->second;
                if (file->fileStream.rdstate() == std::ios_base::in) {
                    return nullptr;
                } else {
                    return file;
                }
            }
        }
    private:
        void CreateDirectories(const std::string& filePath) {
            auto path = std::filesystem::path(filePath);
            std::filesystem::path parentPath = path.parent_path();
            if (!parentPath.empty()) {
                std::filesystem::create_directories(parentPath);
            }
        }
    public:
        size_t Read(File* file, char* buffer, size_t length) override {
            std::lock_guard<std::mutex> lock(mutex_);
            if (file->fileStream.is_open()) {
                file->fileStream.read(buffer, length);
                return file->fileStream.gcount();
            } else {
                return 0;
            }
        }

        size_t Write(File* file, const char* buffer, size_t length) override {
            std::lock_guard<std::mutex> lock(mutex_);
            if (file->fileStream.is_open()) {
                file->fileStream.write(buffer, length);
                return file->fileStream.tellp();
            } else {
                return 0;
            }
        }

        void Close(File* file) override {
            file->fileStream.close();
        }
    };
};

int main() {
    TestTask::VFS vfs;
    TestTask::File* file1 = vfs.Open("file2.txt");
    if (file1 != nullptr) { // �������� ����������� �������� � ������ ������
        std::cout << "File is opened in readonly " << std::endl;
    }

    file1 = vfs.Create("file3.txt");
    if (file1 != nullptr) { // �������� ����������� �������� � ������ ������
        std::cout << "File is opened in writeonly " << std::endl;
    }

    std::thread readThread([&]() { // ������� ������ ��� ������������� ������ � ������
        char buffer[256];
        size_t bytesRead = vfs.Read(file1, buffer, sizeof(buffer)); // ������� ��������� ������ �� ����� � ������� �� ������ � ������
        std::cout << "Read " << bytesRead << " bytes from file." << std::endl;
        });
    const char* data = "Some data";
    std::thread writeThread([&]() {
        size_t bytesWritten = vfs.Write(file1, data, strlen(data)); // ������� ������ ������ � ���� � ������� �� ������ � ������
        std::cout << "Written " << bytesWritten << " bytes to file." << std::endl;
        });
    readThread.join(); // ������� ���������� ������ �������
    writeThread.join();
    vfs.Close(file1); // �������� �����
    return 0;
}
