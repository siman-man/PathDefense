class Summary
  def initialize(num = 100)
    sum = 0 
    min_list = []
    cnt = 0
    File.open(Dir::pwd + '/result.txt', 'r') do |file|
      file.readlines.each do |line|
        if line =~ /N =/
          size = line.chomp.split(' ')[2].to_i

          if size <= 14
            min_list << cnt+1
          end
        end

        if line =~ /Score/
          score = line.chomp.split(' ')[2].to_f

          if score == -1
            puts "Seed #{cnt+1000} is over"
            score = Float::INFINITY
          end
          sum += score
          cnt += 1
        end 
      end 
    end 
    p sum/cnt.to_f
    #p min_list
  end 
end

s = Summary.new(ARGV[0])
