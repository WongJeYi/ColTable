import time
import random
import string
import _ColTable as ct
import numpy as np
from typing import Type, TypeVar, List, Union

T = TypeVar("T", int, float, bool, string)

def random_generator(data_type: Type[T],loop) -> List[T]:
    #  random length
    length = random.randint(1, 100)
    
    vec = []
    
    for _ in range(length):
        vec_inner=[]
        for _ in range(length):
            if data_type is float:
                vec_inner.append(random.uniform(0.0, 10.0))                
            elif data_type is bool:
                vec_inner.append(random.random() < 0.5)
            elif data_type is int:
                vec_inner.append(random.randint(-10, 10))
            elif data_type is str:
                chars = string.ascii_letters + string.digits
                vec_inner.append( ''.join(random.choices(chars, k=length)))    
            else:
                raise TypeError("Unsupported type for random_generator")
        if(loop):
            vec.append(vec_inner)
        else:
            vec=vec_inner
            break
                
    return vec

def multiplication2D(data_type,loop):
    original= random_generator(data_type,loop)

    start_time = time.perf_counter()
    size = len(original)
    mat=[[0.0 for _ in range(size)] for _ in range(size)]
    for i in range(len(original)):
        for j in range(len(original[i])):
            for k in range(len(original[i])):
                mat[i][j]+=original[k][i]*original[i][k]

    end_time = time.perf_counter()
    print(f"Execution time Normal: {end_time - start_time:.6f} seconds")
    if(loop):
        if(data_type is int):
            table = ct.ColTable.from_2d_int64(original)
        elif(data_type is float):
            table = ct.ColTable.from_2d_double(original)
        elif(data_type is bool):
            table = ct.ColTable.from_2d_bool(original)
        elif(data_type is str):
            table = ct.ColTable.from_2d_string(original)
    else:
        if(data_type is int):
            table = ct.ColTable.from_int64(original)
        elif(data_type is float):
            table = ct.ColTable.from_double(original)
        elif(data_type is bool):
            table = ct.ColTable.from_bool(original)
        elif(data_type is str):
            table = ct.ColTable.from_string(original)

    start_time = time.perf_counter()
    size = len(original)
    mat=[[0.0 for _ in range(size)] for _ in range(size)]
    for i in range(len(original)):
        for j in range(len(original[i])):
            for k in range(len(original[i])):
                mat[i][j]+=table[k][i]*table[i][k]
    
    end_time = time.perf_counter()
    print(f"Execution time ColTable: {end_time - start_time:.6f} seconds")

    
    start_time = time.perf_counter()
    arr = np.array(original)
    result = arr @ arr
    
    end_time = time.perf_counter()
    print(f"Execution time Numpy Normal: {end_time - start_time:.6f} seconds")


    start_time = time.perf_counter()
    matrix = table.to_numpy_matrix()
    
    result = matrix @ matrix
    
    end_time = time.perf_counter()
    print(f"Execution time Numpy ColTable: {end_time - start_time:.6f} seconds")

if __name__ =="__main__":
    multiplication2D(float,1)