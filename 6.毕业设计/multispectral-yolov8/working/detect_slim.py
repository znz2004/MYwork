import argparse
import os
import numpy as np
import cv2
import torch
from models.experimental import attempt_load
from utils.general import check_img_size, scale_coords
from ultralytics.utils.ops import non_max_suppression
from utils.datasets import letterbox
from utils.torch_utils import select_device
import warnings

os.environ["KMP_DUPLICATE_LIB_OK"] = "TRUE"
warnings.filterwarnings("ignore")

parser = argparse.ArgumentParser()
# 检测参数
parser.add_argument('--weights', default=r"weights\weights\best.pt", type=str, help='weights path')
parser.add_argument('--image_rgb', default=r"test\rgb", type=str,
                    help='image_rgb')
parser.add_argument('--image_ir', default=r"test\ir", type=str,
                    help='image_rgb')
parser.add_argument('--conf_thre', type=int, default=0.4, help='conf_thre')
parser.add_argument('--iou_thre', type=int, default=0.5, help='iou_thre')
parser.add_argument('--save_image', default=r"./results", type=str, help='save img or video path')
parser.add_argument('--device', type=str, default="0", help='use gpu or cpu')
parser.add_argument('--imgsz', type=int, default=640, help='image size')
parser.add_argument('--merge_nms', default=False, action='store_true', help='merge class')
parser.add_argument('--vis', default=True, action='store_true', help='visualize image')
opt = parser.parse_args()


def get_color(idx):
    idx = idx * 3
    color = ((37 * idx) % 255, (17 * idx) % 255, (29 * idx) % 255)
    return color


class Detector:
    def __init__(self, device, model_path=r'./best.pt', imgsz=640, merge_nms=False):

        self.device = device
        self.model = attempt_load(model_path, map_location=device)  # load FP32 model
        self.names = self.model.names
        self.stride = max(int(self.model.stride.max()), 32)  # grid size (max stride)
        self.imgsz = check_img_size(imgsz, s=self.stride)
        self.merge_nms = merge_nms

    def process_image(self, image, imgsz, stride, device):
        img = letterbox(image, imgsz, stride=stride)[0]
        img = img.transpose((2, 0, 1))[::-1]
        img = np.ascontiguousarray(img)
        img = torch.from_numpy(img).to(device)
        im = img.float()  # uint8 to fp16/32
        im /= 255.0
        im = im[None]
        return im

    @torch.no_grad()
    def __call__(self, image_rgb: np.ndarray, image_ir: np.ndarray, conf, iou):
        img_vis = image_rgb.copy()
        img_vis_ir = image_ir.copy()
        img_rgb = self.process_image(image_rgb, self.imgsz, self.stride, device)
        img_ir = self.process_image(image_ir, self.imgsz, self.stride, device)

        # inference
        pred = self.model(torch.cat([img_rgb, img_ir], dim=1))[0]
        # Apply NMS
        pred = non_max_suppression(pred, conf_thres=conf, iou_thres=iou, classes=None,
                                   agnostic=self.merge_nms)

        for i, det in enumerate(pred):  # detections per image
            # Rescale boxes from img_size to im0 size
            det[:, :4] = scale_coords(img_rgb.shape[2:], det[:, :4], image_rgb.shape).round()
            for *xyxy, conf, cls in reversed(det):
                xmin, ymin, xmax, ymax = xyxy[0], xyxy[1], xyxy[2], xyxy[3]
                cv2.rectangle(img_vis, (int(xmin), int(ymin)), (int(xmax), int(ymax)),
                              get_color(int(cls) + 2), 2)
                cv2.putText(img_vis, f"{self.names[int(cls)]}/{conf:.1f}", (int(xmin), int(ymin - 5)),
                            cv2.FONT_HERSHEY_COMPLEX, 0.5, get_color(int(cls) + 2), thickness=2)
                cv2.rectangle(img_vis_ir, (int(xmin), int(ymin)), (int(xmax), int(ymax)),
                              get_color(int(cls) + 2), 2)
                cv2.putText(img_vis_ir, f"{self.names[int(cls)]}/{conf:.1f}", (int(xmin), int(ymin - 5)),
                            cv2.FONT_HERSHEY_COMPLEX, 0.5, get_color(int(cls) + 2), thickness=2)

        return img_vis, img_vis_ir


if __name__ == '__main__':
    print("开始生成数据")
    device = select_device(opt.device)
    print(device)
    detector = Detector(device=device, model_path=opt.weights, imgsz=opt.imgsz, merge_nms=opt.merge_nms)

    images_format = ['.png', '.jpg', '.txt', '.jpeg']
    image_names = [name for name in os.listdir(opt.image_rgb) for item in images_format if
                   os.path.splitext(name)[1] == item]

    for i, img_name in enumerate(image_names):
        print(i, img_name)
        rgb_path = os.path.join(opt.image_rgb, img_name)
        ir_path = os.path.join(opt.image_ir, img_name)
        img_rgb = cv2.imread(rgb_path)
        img_ir = cv2.imread(ir_path)
        img_vi, img_ir = detector(img_rgb, img_ir, opt.conf_thre, opt.iou_thre)
        # 横向拼接img_vi和img_ir
        img_combined = cv2.hconcat([img_vi, img_ir])
        # 保存拼接后的图像
        cv2.imwrite(os.path.join(opt.save_image, img_name), img_combined)

        if opt.vis:
            cv2.imshow(img_name, img_combined)
            cv2.waitKey(0)
            cv2.destroyAllWindows()
