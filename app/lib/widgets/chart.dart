/// Timeseries chart example
import 'package:charts_flutter/flutter.dart' as charts;
import 'package:flutter/material.dart';

/// Example of a line chart rendered with dash patterns.
class DashPatternLineChart extends StatelessWidget {
  final List<charts.Series> seriesList;
  final bool animate;

  DashPatternLineChart(this.seriesList, {this.animate});

  @override
  Widget build(BuildContext context) {
    return new charts.LineChart(
        seriesList,
        behaviors: [new charts.SeriesLegend()],
        animate: animate,
        domainAxis: new charts.NumericAxisSpec(
            renderSpec: new charts.SmallTickRendererSpec(
                labelStyle: new charts.TextStyleSpec(
                    fontSize: 12, // size in Pts.
                    color: charts.MaterialPalette.white
                ),

                lineStyle: new charts.LineStyleSpec(
                    color: charts.MaterialPalette.white
                )
            )
        ),
        primaryMeasureAxis: new charts.NumericAxisSpec(
          tickFormatterSpec: charts.BasicNumericTickFormatterSpec((value) =>  value.round().toString() + ' %'),
            tickProviderSpec: charts.BasicNumericTickProviderSpec(desiredTickCount: 5),
          renderSpec: new charts.SmallTickRendererSpec(
              labelStyle: new charts.TextStyleSpec(
                  fontSize: 12, // size in Pts.
                  color: charts.MaterialPalette.white
              ),

              lineStyle: new charts.LineStyleSpec(
                  color: charts.MaterialPalette.white
              )
          )
        ),
      secondaryMeasureAxis: new charts.NumericAxisSpec(
          tickFormatterSpec: charts.BasicNumericTickFormatterSpec((value) =>  value.round().toString() + ' b/m'),
          tickProviderSpec: charts.BasicNumericTickProviderSpec(desiredTickCount: 5),
          renderSpec: new charts.SmallTickRendererSpec(
              labelStyle: new charts.TextStyleSpec(
                  fontSize: 12, // size in Pts.
                  color: charts.MaterialPalette.white
              ),

              lineStyle: new charts.LineStyleSpec(
                  color: charts.MaterialPalette.white
              )
          )
      ),
    );
  }

}

/// Sample linear data type.
class LinearSales {
  final int year;
  final int sales;

  LinearSales(this.year, this.sales);
}