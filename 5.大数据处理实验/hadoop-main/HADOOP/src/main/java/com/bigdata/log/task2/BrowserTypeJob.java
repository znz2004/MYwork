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
 * 任务2-2: 用户端类型统计
 */
public class BrowserTypeJob {

    /**
     * Mapper类: 提取浏览器类型并计数
     */
    public static class BrowserTypeMapper extends Mapper<LongWritable, Text, Text, IntWritable> {
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
                
                // 提取浏览器类型
                String browserType = logMap.get("browser_type");
                if (browserType != null && !browserType.isEmpty()) {
                    outKey.set(browserType);
                    context.write(outKey, ONE);
                }
                
            } catch (Exception e) {
                // 异常处理
                context.getCounter("Browser Type", "Parse exception").increment(1);
            }
        }
    }

    /**
     * Reducer类: 汇总每种浏览器类型的用户数量
     */
    public static class BrowserTypeReducer extends Reducer<Text, IntWritable, Text, IntWritable> {
        private IntWritable result = new IntWritable();
        
        @Override
        protected void reduce(Text key, Iterable<IntWritable> values, Context context)
                throws IOException, InterruptedException {
            int sum = 0;
            
            // 累加该浏览器类型的所有计数
            for (IntWritable val : values) {
                sum += val.get();
            }
            
            result.set(sum);
            context.write(key, result);
        }
    }
} 