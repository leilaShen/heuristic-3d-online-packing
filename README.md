# heuristic-3d-online-packing
based on 2d-packing https://github.com/juj/RectangleBinPack   adapt the code to fit 3d
潜在的改进方向：
1. Merge free space 需要一直merge 直到不再变化（原代码中只merge了一次）改进的这个代码我找不到了应该也很好写
2. skyline方法是最好的 但是我一直没想明白怎么扩展到3d ，目前只实现了最容易的断头台算法和MaxRect算法的3d版本
