class Memory:
    def __init__(self):
        self.memory = [None] * 128
        self.free = [[0 , 128]]
        self.firstIndex = {}

    def allocate(self , precess):
        if precess.id in self.firstIndex:
            print(f'process with id {precess.id} is already in the memory!')
            return
        for i in range(len(self.free)):
            dif = self.free[i][1] - self.free[i][0]
            if dif >= precess.size:
                self.firstIndex[precess.id] = self.free[i][0]
                initial = self.free[i][0]
                self.free[i][0] = self.free[i][0] + precess.size
                for j in range(initial , self.free[i][0]):
                    self.memory[j] = precess.id
                return
        print('not enough space')
        return

    def remove(self,process):
        if process.id not in self.firstIndex:
            print(f'process {process.id} not found')
            return
        self.free.sort()
        prev = 0
        next = 1
        while next < len(self.free):
            if self.free[next][0] > self.firstIndex[process.id]:
                break
            prev += 1
            next += 1
        if next < len(self.free):
            self.free[prev][1] = self.free[next][1]
            self.free.remove(self.free[next])
        else:
            self.free.append([self.firstIndex[process.id] , process.size])
            self.free.sort()
        for index in range(self.firstIndex[process.id], self.firstIndex[process.id] + process.size):
            self.memory[index] = None
        del self.firstIndex[process.id]



class Process:
    def __init__(self , id , size):
        self.id = id
        self.size = size


p1 = Process(1 , 5)
p2 = Process(2 , 5)
p3 = Process(3 , 15)
p4 = Process(4 , 15)
p5 = Process(5 , 3)


memory  = Memory()
memory.allocate(p1)
memory.allocate(p3)
memory.remove(p1)
memory.remove(p2)
memory.allocate(p4)
memory.allocate(p5)
print(memory.memory)
print(memory.free)
print(memory.firstIndex)