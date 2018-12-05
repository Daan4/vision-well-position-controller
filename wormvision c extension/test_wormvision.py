import wormvision
import timeit

if __name__ == "__main__":
    rows = 10
    cols = 10
    data = list(range(rows*cols))
    target = (20, 10)

    start = timeit.default_timer()
    wormvision.WBFE_evaluate(data, cols, rows, target)
    stop = timeit.default_timer()

    print('Time: ', stop - start)
