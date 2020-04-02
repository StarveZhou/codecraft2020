import sys


def error_occur(str):
    print(str)
    exit(0)


def read_from_file(path):
    data = []
    size = 0
    with open(path, 'r') as fp:
        lines = fp.readlines()
        size = int(lines[0])
        lines = lines[1:]
        for line in lines:
            data.append(list(map(int, line.split(','))))
    if size != len(data):
        error_occur("error occur from {}, lines require {}, actual {}".format(path, size, len(data)))
    return data


def compare(std, res):
    std_data = read_from_file(std)
    res_data = read_from_file(res)
    if len(std_data) != len(res_data):
        error_occur("diff number of lines, std: {}, actual: {}".format(len(std_data), len(res_data)))
    for i in range(len(std_data)):
        std_line = std_data[i]
        res_line = res_data[i]
        if len(std_line) != len(res_line):
            error_occur(
                "line: {}, diff number of items, std: {}, actual: {}".format(i + 1, len(std_line), len(res_line)))
        for j in range(len(std_line)):
            if std_line[j] != res_line[j]:
                error_occur("line: {}, cow: {}, std: {}, actual: {}".format(i + 1, j + 1, std_line[j], res_line[j]))
    print("success!")


if __name__ == '__main__':
    args = sys.argv
    result_path = "resources/result.txt"
    output_path = "test_output.txt"
    if len(args) >= 2:
        output_path = args[1]
    if len(args) >= 3:
        result_path = args[2]
    compare(result_path, output_path)
