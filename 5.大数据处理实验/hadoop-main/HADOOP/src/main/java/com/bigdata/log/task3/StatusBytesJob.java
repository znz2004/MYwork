package com.bigdata.log.task3;

import java.io.IOException;
import java.util.Map;

import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;

import com.bigdata.log.utils.LogParser;

/**
 * 任务3-1: 状态码与传输数据量关系分析
 */
public class StatusBytesJob {

    /**
     * Mapper类: 提取状态码和传输字节数
     */
    public static class StatusBytesMapper extends Mapper<LongWritable, Text, Text, Text> {
        private Text outKey = new Text();
        private Text outValue = new Text();
        
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
                
                // 提取状态码和传输字节数
                String status = logMap.get("status");
                String bytes = logMap.get("body_bytes_sent");
                
                if (status != null && bytes != null) {
                    try {
                        // 尝试将传输字节数转换为数值
                        long bytesLong = Long.parseLong(bytes);
                        
                        // 使用状态码作为key
                        outKey.set(status);
                        // 使用传输字节数作为value
                        outValue.set(bytes);
                        
                        context.write(outKey, outValue);
                    } catch (NumberFormatException e) {
                        // 如果字节数不是有效数字，则跳过此记录
                        context.getCounter("Status Bytes", "Invalid bytes count").increment(1);
                    }
                }
                
            } catch (Exception e) {
                // 异常处理
                context.getCounter("Status Bytes", "Parse exception").increment(1);
            }
        }
    }

    /**
     * Reducer类: 计算每个状态码的平均传输字节数
     */
    public static class StatusBytesReducer extends Reducer<Text, Text, Text, Text> {
        private Text result = new Text();
        
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context)
                throws IOException, InterruptedException {
            long sum = 0;
            int count = 0;
            
            // 累加该状态码的所有传输字节数
            for (Text val : values) {
                try {
                    long bytes = Long.parseLong(val.toString());
                    sum += bytes;
                    count++;
                } catch (NumberFormatException e) {
                    // 忽略无效数据
                }
            }
            
            // 计算平均传输字节数
            double average = (count > 0) ? (double) sum / count : 0;
            
            result.set(String.valueOf(average));
            context.write(key, result);
        }
    }
} 