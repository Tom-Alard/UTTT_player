import torch
from torch import nn

NUM_INPUTS = 190


class NeuralNetwork(nn.Module):
    def __init__(self):
        super(NeuralNetwork, self).__init__()
        self.l1 = nn.Linear(NUM_INPUTS, 256)
        self.l2 = nn.Linear(256, 32)
        self.l3 = nn.Linear(32, 32)
        self.l4 = nn.Linear(32, 1)

    def forward(self, x):
        x = self.l1(x)
        x = torch.clamp(x, min=0, max=1)
        x = self.l2(x)
        x = torch.clamp(x, min=0, max=1)
        x = self.l3(x)
        x = torch.clamp(x, min=0, max=1)
        x = self.l4(x)
        x = torch.clamp(x, min=-0.5, max=0.5)
        return x


class WeightClipper(object):
    def __init__(self, frequency=5):
        self.frequency = frequency

    def __call__(self, module):
        if hasattr(module, 'weight') and module.weight.shape != (256, NUM_INPUTS):
            module.weight.data = module.weight.data.clamp(-2, 2)
