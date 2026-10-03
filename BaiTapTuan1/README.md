# Bài tập tuần 1 — MFC Dialog cơ bản, Visual Studio 2019

Đây là project MFC dạng Dialog, giao diện nằm trong resource `.rc` để mở và
kéo thả trong Resource Editor. Logic chọn folder, Save và Load nằm trong
`MFCFileWeek1Dlg.cpp`. Không có lớp helper đọc/ghi file riêng.

## Mở và chạy

1. Giải nén toàn bộ ZIP vào một thư mục mới.
2. Mở `BaiTapTuan1/MFCFileWeek1.sln` bằng Visual Studio 2019.
3. Chọn **Debug | x64** hoặc **Debug | Win32**.
4. Nhấn **Ctrl + F5** để build và chạy.

Project dùng toolset **v142**, Unicode và MFC dùng DLL. Cần cài MSVC v142,
C++ MFC cho v142 (x86 & x64) và Windows SDK trong Visual Studio Installer.
Precompiled header được tắt nên không cần `pch.h`, `pch.cpp` hoặc `framework.h`.

## Kéo thả giao diện

Trong Visual Studio:

1. Chọn **View → Other Windows → Resource View** (Ctrl + Shift + E).
2. Mở **MFCFileWeek1 → Dialog → IDD_MFCFILEWEEK1_DIALOG**.
3. Kéo thả, di chuyển hoặc chỉnh kích thước các control như project MFC thông thường.
4. Chọn control, mở Properties để xem ID, Caption và các thuộc tính.

Giao diện đã có đúng các thành phần trong đề:

| Control | ID | Thuộc tính chính |
| --- | --- | --- |
| Edit nhập nội dung | IDC_EDIT_CONTENT | Multiline, Want Return, Auto VScroll, Vertical Scroll |
| Edit hiển thị đường dẫn | IDC_EDIT_PATH | Read Only, Auto HScroll |
| Button chọn folder | IDC_BTN_FOLDER | Caption: Chon folder... |
| Button lưu | IDC_BTN_SAVE | Caption: Save |
| Button đọc | IDC_BTN_LOAD | Caption: Load |

Giữ các ID trên nếu chỉ chỉnh bố cục, vì code dùng ID để kết nối với control.
Hai Static label dùng ID mặc định IDC_STATIC.

## Vì sao có các file .h?

| File | Vai trò |
| --- | --- |
| MFCFileWeek1.h | Khai báo lớp ứng dụng |
| MFCFileWeek1Dlg.h | Khai báo lớp dialog, hai biến và các hàm xử lý nút |
| resource.h | Định nghĩa ID của dialog và control |

Đây là ba header phục vụ cấu trúc MFC của bài này. Không có WinApiFile.h,
WinHelpers.h hoặc header riêng cho từng chức năng.

`MFCFileWeek1.cpp` mở dialog chính khi ứng dụng khởi động.
`MFCFileWeek1Dlg.cpp` chứa toàn bộ xử lý giao diện và đọc/ghi.
`MFCFileWeek1.rc` chứa bố cục giao diện; `.sln` và `.vcxproj` chứa cấu hình project.

## Code cần học

- `DDX_Text`: kết nối hai Edit control với hai biến CString.
- `UpdateData(TRUE)`: lấy dữ liệu từ giao diện vào biến.
- `UpdateData(FALSE)`: đưa dữ liệu từ biến ra giao diện.
- `ON_BN_CLICKED`: kết nối thao tác bấm nút với hàm xử lý.
- `OnChooseFolder`: dùng CFolderPickerDialog để chọn thư mục.
- `OnSave`: dùng CreateFileW + WriteFile + CloseHandle để lưu.
- `OnLoad`: dùng CreateFileW + ReadFile + CloseHandle để đọc lại.

MFC dùng để làm giao diện; đọc/ghi tệp gọi trực tiếp WinAPI.
Hai dòng chuyển đổi UTF-8 giúp giữ được nội dung tiếng Việt khi lưu ra file.

## Kiểm tra bài

Nhập nội dung → chọn folder → Save → xóa nội dung ô nhập → Load.
Nội dung đã lưu phải xuất hiện lại.

Tên file cố định là `HelloWorld.txt` trong folder đã chọn. Save ghi đè nội dung cũ;
Load thay thế nội dung đang hiển thị. Bản cơ bản đọc file UTF-8 do chương trình
lưu, với dung lượng tối đa 1 MiB. Nếu chưa chọn folder hoặc không mở được file,
chương trình hiển thị thông báo.

Đã kiểm tra cấu trúc project, tham chiếu file, ID control và message map.
Chưa build/chạy trên Windows vì môi trường tạo bài không có Visual Studio/MFC.

Nguồn đọc/ghi file:
https://learn.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing
