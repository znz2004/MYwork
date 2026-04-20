package com.bigdata.log.task3;

import java.io.IOException;
import java.util.Map;

import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;

import com.bigdata.log.utils.LogParser;

/**
 * 任务3-2: 小时级错误率波动分析
 */
public class HourlyErrorRateJob {

    /**
     * Mapper类: 统计每小时的请求总数和错误请求数
     */
    public static class ErrorRateMapper extends Mapper<LongWritable, Text, Text, Text> {
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
                
                // 提取时间和状态码
                String dateHour = logMap.get("date_hour");
                String status = logMap.get("status");
                
                if (dateHour != null && !dateHour.equals("Unknown") && status != null) {
                    outKey.set(dateHour);
                    
                    // 检查是否为错误状态码（4xx或5xx）
                    boolean isError = status.startsWith("4") || status.startsWith("5");
                    
                    // 输出格式: 总请求数,错误请求数
                    outValue.set("1," + (isError ? "1" : "0"));
                    context.write(outKey, outValue);
                }
                
            } catch (Exception e) {
                // 异常处理
                context.getCounter("Error Rate", "Parse exception").increment(1);
            }
        }
    }

    /**
     * Reducer类: 计算每小时的错误率
     */
    public static class ErrorRateReducer extends Reducer<Text, Text, Text, Text> {
        private Text result = new Text();
        
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context)
                throws IOException, InterruptedException {
            int totalRequests = 0;
            int errorRequests = 0;
            
            // 累加总请求数和错误请求数
            for (Text val : values) {
                String[] parts = val.toString().split(",");
                if (parts.length == 2) {
                    totalRequests += Integer.parseInt(parts[0]);
                    errorRequests += Integer.parseInt(parts[1]);
                }
            }
            
            // 计算错误率
            double errorRate = (totalRequests > 0) ? (double) errorRequests / totalRequests : 0;
            
            // 格式化错误率为百分比
            String errorRateStr = String.format("%.4f", errorRate);
            
            result.set(errorRateStr);
            context.write(key, result);
        }
    }
} 