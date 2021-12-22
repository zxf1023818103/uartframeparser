# ���ڵ��Թ���

## һ��Windows �±��밲װ����

### 1. ��װ Visual Studio Community 2022

[���� Visual Studio Community 2022 ��װ����](https://visualstudio.microsoft.com/zh-hans/vs/community/)���򿪣���ѡ��������ѡ��еġ�ʹ�� C++ �����濪�������������ѡ��еġ������� Windows �� Git�������԰�ѡ��еġ�Ӣ�����װ������ϵͳ��

![��װ Visual Studio](./doc/��װ Visual Studio.png)

![������ Windows �� Git](./doc/������ Windows �� Git.png)

![image-20211221124010753](./doc/Ӣ�����԰�.png)

### 2. ��װ vcpkg �������߲���װ����

ѡ��һ���ļ�����Ϊ vcpkg �������ߵİ�װ�ļ��У��������л������ļ����£�ִ���������

``` powershell
PS C:\Users\tuya> git clone "https://github.com/Microsoft/vcpkg.git"
PS C:\Users\tuya> cd vcpkg
PS C:\Users\tuya\vcpkg> .\bootstrap-vcpkg.bat
PS C:\Users\tuya\vcpkg> .\vcpkg.exe integrate install
PS C:\Users\tuya\vcpkg> .\vcpkg.exe install cjson:x64-windows lua:x64-windows gtest:x64-windows qtbase:x64-windows qttools:x64-windows qtserialport:x64-windows
```

### 3. �޸ĵ��������ļ�

����Ŀ�ļ����� `.vs` �ļ��е� `launch.vs.json` �ļ������ļ��е� `C:\\Users\\tuya\\vcpkg` �滻����� vcpkg ʵ�ʰ�װĿ¼��

```json
{
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "quartframeparser.exe",
      "name": "quartframeparser.exe",
      "env": {
        "QT_QPA_PLATFORM_PLUGIN_PATH": "C:\\Users\\tuya\\vcpkg\\installed\\x64-windows\\Qt6\\plugins\\platforms"
      }
    },
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "quartframeparserd.exe",
      "name": "quartframeparserd.exe",
      "env": {
        "QT_QPA_PLATFORM_PLUGIN_PATH": "C:\\Users\\tuya\\vcpkg\\installed\\x64-windows\\debug\\Qt6\\plugins\\platforms"
      }
    }
  ]
}
```

### 4. ��������

ʹ���ļ���Դ����������Ŀ�ļ��У��Ҽ���Ŀ�ļ��пհ״���ѡ�� Open with Visual Studio���� Visual Studio ����Ŀ���ڽ����Ϸ�ѡ�����Ŀ��Ϊ `quartframeparser.exe` ��Release ģʽ�£��� `quartframeparserd.exe` ��Debug ģʽ�£�����������Ŀ��

## ����ͼ�ν���ʹ��˵��

�򿪳���󣬲˵������ File��ѡ�� Settings �������ý��档�����ý����� Refresh ��⴮���б�ѡ���Ӧ���ڼ������ʣ�

![image-20211222120005411](./doc/���ý���.png)

���ı���������֡��ʽ�����ļ�·�������ߵ�� Open ���ļ�ѡ��Ի�����ѡ��֡��ʽ�����ļ������ OK Ӧ�����á�

![image-20211222120303923](./doc/�ļ�ѡ��.png)

![image-20211222120437704](./doc/�������.png)

��Ϊ���ս��棬�·���ʾ���յ��Ĵ���֡ ��˫���ɲ鿴֡�ṹ��֡�ֶΣ�˫��֡�ֶοɲ鿴֡�ֶε���ϸ��Ϣ����֡�ֶγ��ȡ�֡�ֶ������ȣ�

![image-20211222120558982](./doc/���ս���.png)

��Ϊ���ͽ��棬��� Add Byte ��ť�������ֽڣ�ѡ��ĳһ�ֽں��� Insert Byte �� Remove Byte ���ڸ��ֽ�ǰ�����ֽڻ���ɾ�����ֽڡ�˫���ֽڿɱ༭���༭��ɺ��� Send �����ֽڣ����ͼ�¼��ʾ���·��б��У�˫�����ͼ�¼�����±༭�����͡�

![image-20211222123731183](./doc/���ͽ���.png)

## ����Bug �б�

- [ ] ������ʾ���Ļ�����ʺš��������Ʋ������ڱ��뷽ʽ���µģ���Ҳ�п������������⣬Ŀǰ�����Ų��С�
