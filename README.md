# NT-LLM
This repository contains the source code for the paper **From Anchors to Answers: A Novel Node Tokenizer for
Integrating Graph Structure into Large Language Models**.

## Environment Setup
```bash
conda create --name ntllm python=3.9 -y
conda activate ntllm

# https://pytorch.org/get-started/locally/
conda install pytorch==2.0.1 torchvision==0.15.2 torchaudio==2.0.2 pytorch-cuda=11.8 -c pytorch -c nvidia
python -c "import torch; print(torch.__version__)"
python -c "import torch; print(torch.version.cuda)"
pip install pyg_lib torch_scatter torch_sparse torch_cluster torch_spline_conv -f https://data.pyg.org/whl/torch-2.0.1+cu118.html

pip install peft
pip install pandas
pip install ogb
pip install transformers
pip install wandb
pip install sentencepiece
pip install torch_geometric
pip install datasets
pip install pcst_fast
pip install gensim
pip install scipy==1.12
pip install protobuf
```

## Data Preprocessing
```bash
python preprocess.py
```

## Reference 
```
@article{node-tokenizer,
  author       = {Yanbiao Ji and
                  Chang Liu and
                  Xin Chen and
                  Yue Ding and
                  Dan Luo and
                  Mei Li and
                  Wenqing Lin and
                  Hongtao Lu},
  title        = {{NT-LLM:} {A} Novel Node Tokenizer for Integrating Graph Structure
                  into Large Language Models},
  journal      = {CoRR},
  volume       = {abs/2410.10743},
  year         = {2024}
}
```

## training
```bash
python main.py --dataset arxiv
```
