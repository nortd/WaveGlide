import 'package:flutter/material.dart';

class ValueTile extends StatelessWidget {
  Object value;
  String description;
  Color color;
  double size;


  ValueTile({ this.value, this.description, this.color, this.size });

  String _setTileValue(Object value) {
    if(value == null) {
      return '-';
    }
    return value.toString();
  }

  @override
  Widget build(BuildContext context) {
    if(value == null) {
      color = Colors.grey;
    }
    return Container(
      margin: EdgeInsets.all(8),
      width: size - 16,
      height: size - 16,
      alignment: FractionalOffset.center,
      decoration: BoxDecoration(
        borderRadius: BorderRadius.all(Radius.circular(8)),
        color: this.color,
      ),
      child: Center(
        child: Column(
          children: <Widget> [
              SizedBox(height: 15,),
              new Text(_setTileValue(this.value), overflow: TextOverflow.fade, maxLines: 1, softWrap: false, textAlign: TextAlign.center, textScaleFactor: 3),
              new Text(this.description, textAlign: TextAlign.center, textScaleFactor: 1.1, overflow: TextOverflow.fade, maxLines: 1, softWrap: false,),
            ]
        )
      )
    );

  }
}