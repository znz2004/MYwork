package com.bigdata.log.task2;

import java.io.IOException;
import java.util.Map;

import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;

import com.bigdata.log.utils.LogParser;

/**
 * 任务2-1: 每小时网站浏览量统计
 */
public class HourlyVisitJob {

    /**
     * Mapper类: 提取每条日志的时间并计数
     */
    public static class HourlyVisitMapper extends Mapper<LongWritable, Text, Text, IntWritable> {
        private Text outKey = new Text();
        private static final IntWritable ONE = new IntWritable(1);
        
        @Override
        protected void map(LongWritable key, Text value, Context context)
                throws IOException, InterruptedException {
            // 获取日志行内容
            String line = value.toString();
            
            // 过滤空行或格式不正确的行
            if (line == null || line.trim().isEmpty()) {
                return;
            }
            
            try {
                // 使用LogParser工具类解析日志行
                Map<String, String> logMap = LogParser.parseLogLine(line);
                
                // 检查解析是否成功
                if (logMap.containsKey("error")) {
                    return;
                }
                
                // 提取"date_hour"字段，作为小时级访问量统计的键
                String dateHour = logMap.get("date_hour");
                if (dateHour != null && !dateHour.equals("Unknown")) {
                    outKey.set(dateHour);
                    context.write(outKey, ONE);
                }
                
            } catch (Exception e) {
                // 异常处理
                context.getCounter("Hourly Visit", "Parse exception").increment(1);
            }
        }
    }

    /**
     * Reducer类: 汇总每小时的访问量
     */
    public static class HourlyVisitReducer extends Reducer<Text, IntWritable, Text, IntWritable> {
        private IntWritable result = new IntWritable();
        
        @Override
        protected void reduce(Text key, Iterable<IntWritable> values, Context context)
                throws IOException, InterruptedException {
            int sum = 0;
            
            // 累加该小时的所有访问计数
            for (IntWritable val : values) {
                sum += val.get();
            }
            
            result.set(sum);
            context.write(key, result);
        }
    }
} 