@filename = "PathDefense"

task :default do
  `g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp`
end

task :run do
  system("g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp")
  system("java -jar ./#{@filename}Vis.jar -seed 2 -exec './#{@filename}'")
  #system("java -jar ./#{@filename}Vis.jar -side 12 -seed 105 -exec './#{@filename}'")
end

task :windows do
  system("g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp")
  system("java -jar ./#{@filename}Vis.jar -seed 4 -exec './#{@filename}.exe'")
end

task :one do
  system("g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp")
  system("time java -jar #{@filename}Vis.jar -seed 3 -novis -exec './#{@filename}'")
end

task :two do
  system("g++ -o #{@filename} #{@filename}.cpp")
  system("time java -jar #{@filename}Vis.jar -seed 8 -novis -exec './#{@filename}'")
end

task :novis do
  system('rm result.txt')
  system("g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp")
  1.upto(100) do |num|
    p num
    system("time java -jar ./#{@filename}Vis.jar -seed #{num} -novis -exec './#{@filename}' >> result.txt")
  end
  system('ruby analysis.rb 100')
end

task :final do
  system('rm result.txt')
  system("g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp")
  1.upto(1000) do |num|
    p num
    system("time java -jar ./#{@filename}Vis.jar -seed #{num} -novis -exec './#{@filename}' >> result.txt")
    sleep(1.0)
  end
  system('ruby analysis.rb 1000')
end

task :sample do
  system('rm result.txt')
  system("g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp")
  1.upto(10) do |num|
    p num
    system("time java -jar ./#{@filename}Vis.jar -seed #{num} -novis -exec './#{@filename}' >> result.txt")
  end
  system('ruby analysis.rb 10')
end

task :select do
  system('rm result.txt')
  system("g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp")
  array = [1, 5, 7, 13, 26, 111, 145, 159, 193, 228, 233, 235, 236, 264, 268, 269, 280, 291, 293, 305, 313, 337, 358, 361, 364, 367, 372, 377, 381, 391, 406, 447, 464, 485, 493, 514, 516, 525, 539, 547, 556, 573, 580, 606, 613, 625, 629, 667, 706, 738, 766, 773, 779, 794, 797, 806, 810, 842, 866, 870, 885, 890, 893, 900, 913, 917, 931, 936, 941, 950, 988]
  array.take(10).each do |num|
    p num
    system("time java -jar ./#{@filename}Vis.jar -seed #{num} -novis -exec './#{@filename}' >> result.txt")
  end
  system("ruby analysis.rb #{array.size}")
end

task :test do
  system("g++ -o #{@filename} #{@filename}.cpp")
  system("./#{@filename} < test_case.txt")
end

task :java do
  system("g++ -W -Wall -Wno-sign-compare -O2 -o #{@filename} #{@filename}.cpp")
  system("java CirclesSeparationVis -seed 87 -exec './CirclesSeparation'")
end
