# GPU 像素着色器渲染矢量图形
[Slug Algorithm](https://sluglibrary.com/)
用低多边形绘制精细路径图形
![image](https://github.com/Gaazar/RLL/blob/main/readme/1.png)
![image](https://github.com/Gaazar/RLL/blob/main/readme/2.png)
![image](https://github.com/Gaazar/RLL/blob/main/readme/3.png)
![image](https://github.com/Gaazar/RLL/blob/main/readme/4.png)
![image](https://github.com/Gaazar/RLL/blob/main/readme/5.png)
# 流程
FreeType读取字体-------------------------------+
        ↓                                     |
harfbuzz读取度量，包括                          |
文本方向、kerning、----------+                  |
ligature等特性              |                  |
                           +-->排版-->确定字形--+
给定文本--------------------+     ↓             ↓
                            确定字符位置    读取字形轮廓(二次贝塞尔曲线组)
                                 ↓             ↓                |
                         确定transform矩阵  生成包围曲线组的凸包    |
                                 |             ↓                |
                                 |        化简凸包为最多6个顶点    |
                                 |             ↓                ↓
                                 |         生成多边形      上传曲线信息到gpu
                                 |             |                |
                                 +-------------+----------------+
                                 ↓
                                绘制