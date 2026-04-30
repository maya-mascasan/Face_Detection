# 👱‍♂️ Face Detection via Classical Computer Vision

This project implements a robust face detection pipeline using pure classical computer vision techniques in C++ and OpenCV. Rather than relying on modern Deep Learning models (like Haar Cascades or YOLO), this algorithm detects human faces from scratch using color space transformations, morphological operations, Breadth-First Search (BFS), and geometric shape analysis.

## 🚀 The Detection Pipeline

The algorithm processes images through four distinct stages:

### 1. Color Segmentation (HSV)
The input BGR image is converted to the **HSV (Hue, Saturation, Value)** color space. By isolating the Hue and Saturation channels, the algorithm effectively thresholds human skin tones while ignoring variations in lighting and shadows.

### 2. Morphological Cleanup
To eliminate background noise and refine the skin mask, the algorithm applies aggressive **Closing** operations (multiple Dilations followed by Erosions). This successfully closes gaps in the mask caused by non-skin facial features (like eyes, eyebrows, and the mouth) without expanding the overall size of the face blob.

### 3. Connected Component Labeling (BFS)
A custom **Breadth-First Search (BFS)** algorithm scans the binary mask to find and group connected white pixels into independent blobs. During this traversal, the algorithm dynamically calculates:
* The `Area` of the blob (total pixels).
* The `Bounding Box` coordinates (`min_r`, `max_r`, `min_c`, `max_c`).

### 4. Geometric Shape Analysis (The Filter)
To distinguish a human face from other patches of exposed skin (like hands, arms, or necks), the algorithm applies strict mathematical rules based on human proportions:
* **Size Filter:** Rejects noise and small body parts (Area > 5000 pixels).
* **Aspect Ratio:** Ensures the bounding box proportions match a human head (Width / Height between `0.4` and `1.5` to account for hair and ears).
* **Fill Ratio (Extent):** Calculates how much of the bounding box is filled by the blob (`Blob Area / Bounding Box Area`). Because a face is an oval, it reliably fills between `45%` and `85%` of its box. This flawlessly ignores irregular shapes like bent arms.
* **Positioning:** If multiple blobs pass all mathematical checks, the algorithm selects the highest one in the frame (lowest Y-coordinate).

Finally, a tight bounding box is drawn around the detected face.

## 🛠️ Tech Stack
* **Language:** C++
* **Library:** OpenCV (v4.9.0)
* **Environment:** Visual Studio 2022

## 🧠 Key Learnings
* **Circularity vs. Fill Ratio:** Initially, Thinness Ratio (Circularity) was tested to detect the oval shape of the face. However, because facial features create holes in the binary mask, the perimeter calculation spiked, causing false negatives (the "Swiss Cheese" problem). Switching to **Fill Ratio (Extent)** proved to be infinitely more stable for masks with internal contours.
* **Algorithmic Efficiency:** By calculating Area and Bounding Box coordinates *during* the BFS traversal, the program avoids redundant loops and highly optimizes the shape analysis step.

## 📸 Example Results

Here is the algorithm successfully detecting a face while ignoring the neck:
![Detection Result](result1.jpg)
![Detection Result](result2.jpg)
![Detection Result](result3.jpg)
