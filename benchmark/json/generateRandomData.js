import { random } from '../random.js';

export function generateRandomData(numObjects = 5000, maxDepth = 5) {
  function getRandomInt(min, max) {
    return Math.floor(random() * (max - min + 1)) + min;
  }

  function getRandomName() {
    const firstName = getRandomText(getRandomInt(3, 10)).trim();
    const lastName = getRandomText(getRandomInt(3, 15)).trim();
    return firstName.charAt(0).toUpperCase() + firstName.slice(1) + ' ' +
      lastName.charAt(0).toUpperCase() + lastName.slice(1);
  }

  function getRandomText(length) {
    const chars = 'abcdefghijklmnopqrstuvwxyz ';
    let result = '';
    for (let i = 0; i < length; i++) {
      result += chars.charAt(getRandomInt(0, chars.length - 1));
    }
    return result;
  }

  function getRandomDate() {
    const start = new Date(2000, 0, 1);
    const end = new Date(2022, 11, 31);
    return new Date(start.getTime() + random() * (end.getTime() - start.getTime()));
  }

  const schemas = {
    "User": {
      "name": "string:name",
      "age": "integer",
      "bio": "string:long",
      "registeredAt": "date",
      "interests": "array:string:short",
      "orders": "array:Order"
    },
    "Product": {
      "name": "string:short",
      "description": "string:long",
      "price": "integer"
    },
    "Order": {
      "date": "date",
      "total": "integer",
      "products": "array:Product"
    }
  };

  let objectCount = 0;

  function generateObject(type, depth) {
    objectCount++;

    let splitIdx = type.indexOf(':');
    splitIdx = splitIdx === -1 ? type.length : splitIdx;
    const descriptor = [type.slice(0, splitIdx), type.slice(splitIdx + 1)];

    switch (descriptor[0]) {
      case "string":
        return descriptor[1] === "long" ? getRandomText(getRandomInt(50, 200)) :
          descriptor[1] === "short" ? getRandomText(getRandomInt(5, 20)) :
            getRandomName();
      case "integer":
        return getRandomInt(1, 100);
      case "date":
        return getRandomDate().toISOString().split('T')[0];
      case "array":
        if (depth < maxDepth) {
          let arrayLength = getRandomInt(0, 5);
          const obj = [];
          for (let i = 0; i < arrayLength; i++) {
            obj.push(generateObject(descriptor[1], depth + 1));
          }
          return obj;
        }
        return [];
      default:
        const schema = schemas[descriptor[0]];
        const obj = {};
        for (let key in schema) {
          obj[key] = generateObject(schema[key], depth + 1);
        }
        return obj;
    }
  }

  let data = [];
  while (objectCount < numObjects) {
    data.push(generateObject("User", 0));
  }

  return data;
}