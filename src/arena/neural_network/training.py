import math
import time
import torch
from torch import nn
from torch.utils.data import IterableDataset, DataLoader
import numpy as np
from neural_network.network import NeuralNetwork, WeightClipper


class PositionDataset(IterableDataset):
    POSITIONS_LINE_LENGTH = 24
    EVALUATIONS_LINE_LENGTH = 6

    def __init__(self, is_train: bool, batch_size: int):
        self.batch_size = batch_size
        base_filename = f"../{'train' if is_train else 'test'}_compressed"
        positions_filename = f'{base_filename}_positions.csv'
        evaluations_filename = f'{base_filename}_evaluations.csv'
        with open(positions_filename, 'rb') as f:
            f.seek(0, 2)
            self.amount_of_lines = f.tell() // self.POSITIONS_LINE_LENGTH
            f.seek(0)
            self.positions = np.fromfile(f, dtype=np.uint8).reshape((self.amount_of_lines, self.POSITIONS_LINE_LENGTH))
            self.positions = torch.from_numpy(self.positions)
        with open(evaluations_filename, 'rb') as f:
            evals = []
            while e := f.read(self.EVALUATIONS_LINE_LENGTH):
                evals.append(float(e) - 0.5)
            self.evaluations = np.array(evals, dtype=np.float32)
            self.evaluations = torch.from_numpy(self.evaluations)

    def __len__(self):
        return self.amount_of_lines

    def __iter__(self):
        end = self.amount_of_lines - self.batch_size
        worker_info = torch.utils.data.get_worker_info()
        if worker_info is None:
            iter_start = 0
            iter_end = end
        else:
            per_worker = int(math.ceil(end / float(worker_info.num_workers)))
            worker_id = worker_info.id
            iter_start = worker_id * per_worker
            iter_end = min(iter_start + per_worker, end)
        for i in range(iter_start, iter_end, self.batch_size):
            X = np.unpackbits(self.positions[i:i+self.batch_size], axis=1)[:, :190].astype(np.float32)
            y = self.evaluations[i:i+self.batch_size].reshape((self.batch_size, 1))
            yield X, y


def train_loop(dataloader, model, loss_fn, optimizer, scheduler, batch_size):
    train_loss = 0
    clipper = WeightClipper()
    for i, (X, y) in enumerate(dataloader):
        X, y = X.cuda(), y.cuda()
        pred = model(X)
        loss = loss_fn(pred, y)
        train_loss += loss.item()
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        if i % clipper.frequency == 0:
            model.apply(clipper)
    num_batches = len(dataloader) // batch_size
    train_loss /= num_batches
    scheduler.step(train_loss)
    print(f'Train average loss: {train_loss:>8f}')


def test_loop(dataloader, model, loss_fn, batch_size):
    test_loss = 0
    with torch.no_grad():
        for X, y in dataloader:
            X, y = X.cuda(), y.cuda()
            pred = model(X)
            test_loss += loss_fn(pred, y).item()
    num_batches = len(dataloader) // batch_size
    test_loss /= num_batches
    print(f'Test average loss: {test_loss:>8f}')


def main():
    learning_rate = 0.4
    batch_size = 16384
    epochs = 600

    training_data = PositionDataset(True, batch_size)
    # testing_data = PositionDataset(False, batch_size)
    train_dataloader = DataLoader(training_data, batch_size=1, num_workers=8, persistent_workers=True,
                                  pin_memory=True)
    # test_dataloader = DataLoader(testing_data, batch_size=1)
    model = NeuralNetwork().cuda()
    loss_fn = nn.MSELoss()
    optimizer = torch.optim.SGD(model.parameters(), lr=learning_rate, momentum=0.9, nesterov=True)
    scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(optimizer, patience=3, threshold=0.0002, factor=0.5)
    for i in range(epochs):
        print(f'Epoch {i+1}')
        start = time.time()
        train_loop(train_dataloader, model, loss_fn, optimizer, scheduler, batch_size)
        # test_loop(test_dataloader, model, loss_fn, batch_size)
        print(f'Epoch completed in {time.time() - start:<7f}\n')
        torch.save(model, 'model_latest.pth')


if __name__ == '__main__':
    main()
