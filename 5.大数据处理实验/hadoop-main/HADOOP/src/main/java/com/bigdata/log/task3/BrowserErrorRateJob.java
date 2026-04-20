package com.bigdata.log.task3;

import java.io.IOException;
import java.util.Map;

import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;

import com.bigdata.log.utils.LogParser;

/**
 * 任务3-3: 高风险用户类型识别
 */
public class BrowserErrorRateJob {

    /**
     * Mapper类: 统计每种浏览器类型的请求总数、错误请求数和传输数据量
     */
    public static class BrowserErrorRateMapper extends Mapper<LongWritable, Text, Text, Text> {
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
                
                // 提取浏览器类型、状态码和传输字节数
                String browserType = logMap.get("browser_type");
                String status = logMap.get("status");
                String bytes = logMap.get("body_bytes_sent");
                
                if (browserType != null && status != null && bytes != null) {
                    try {
                        // 尝试将传输字节数转换为数值
                        long bytesLong = Long.parseLong(bytes);
                        
                        // 检查是否为错误状态码（4xx或5xx）
                        boolean isError = status.startsWith("4") || status.startsWith("5");
                        
                        // 使用浏览器类型和状态码的组合作为key
                        outKey.set(browserType + "->" + status);
                        
                        // 输出格式: 总请求数,错误请求数,传输字节数
                        outValue.set("1," + (isError ? "1" : "0") + "," + bytesLong);
                        
                        context.write(outKey, outValue);
                    } catch (NumberFormatException e) {
                        // 如果字节数不是有效数字，则跳过此记录
                        context.getCounter("Browser Error Rate", "Invalid bytes count").increment(1);
                    }
                }
                
            } catch (Exception e) {
                // 异常处理
                context.getCounter("Browser Error Rate", "Parse exception").increment(1);
            }
        }
    }

    /**
     * Reducer类: 计算每种浏览器类型的错误率和平均传输数据量
     */
    public static class BrowserErrorRateReducer extends Reducer<Text, Text, Text, Text> {
        private Text result = new Text();
        
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context)
                throws IOException, InterruptedException {
            int totalRequests = 0;
            int errorRequests = 0;
            long totalBytes = 0;
            
            // 累加总请求数、错误请求数和传输字节数
            for (Text val : values) {
                String[] parts = val.toString().split(",");
                if (parts.length == 3) {
                    totalRequests += Integer.parseInt(parts[0]);
                    errorRequests += Integer.parseInt(parts[1]);
                    totalBytes += Long.parseLong(parts[2]);
                }
            }
            
            // 计算错误率和平均传输字节数
            double errorRate = (totalRequests > 0) ? (double) errorRequests / totalRequests : 0;
            double avgBytes = (totalRequests > 0) ? (double) totalBytes / totalRequests : 0;
            
            // 格式化结果
            String resultStr = String.format("%.4f,%.2f", errorRate, avgBytes);
            
            result.set(resultStr);
            context.write(key, result);
        }
    }
} 