def main():
    with open("sample.txt", "w") as f:
        for i in range(1, 51):
            # print(i)
            # print(f"{i:05d}", end=" ")
            num_str = f"{i:05d}"
            f.write(num_str + " ")

if __name__ == "__main__":
    main()


